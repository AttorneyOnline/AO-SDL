#include <gtest/gtest.h>

#include "metrics/Metrics.h"
#include "metrics/MetricsRegistry.h"
#include "metrics/PrometheusFormatter.h"

using namespace metrics;

// -- Counter ------------------------------------------------------------------

TEST(MetricsCounter, StartsAtZero) {
    Counter c;
    EXPECT_EQ(c.value(), 0.0);
}

TEST(MetricsCounter, Inc) {
    Counter c;
    c.inc();
    EXPECT_EQ(c.value(), 1.0);
    c.inc();
    EXPECT_EQ(c.value(), 2.0);
}

TEST(MetricsCounter, IncN) {
    Counter c;
    c.inc(100);
    EXPECT_EQ(c.value(), 100.0);
    c.inc(50);
    EXPECT_EQ(c.value(), 150.0);
}

// -- Gauge --------------------------------------------------------------------

TEST(MetricsGauge, StartsAtZero) {
    Gauge g;
    EXPECT_EQ(g.value(), 0.0);
}

TEST(MetricsGauge, Set) {
    Gauge g;
    g.set(42.5);
    EXPECT_DOUBLE_EQ(g.value(), 42.5);
    g.set(-1.0);
    EXPECT_DOUBLE_EQ(g.value(), -1.0);
}

TEST(MetricsGauge, IncDec) {
    Gauge g;
    g.inc();
    EXPECT_DOUBLE_EQ(g.value(), 1.0);
    g.dec();
    EXPECT_DOUBLE_EQ(g.value(), 0.0);
    g.inc(10.0);
    g.dec(3.0);
    EXPECT_DOUBLE_EQ(g.value(), 7.0);
}

TEST(MetricsGauge, Negative) {
    Gauge g;
    g.dec(5.0);
    EXPECT_DOUBLE_EQ(g.value(), -5.0);
}

// -- MetricFamily -------------------------------------------------------------

TEST(MetricFamily, LabellessCounter) {
    CounterFamily f("test_counter", "help", {});
    f.get().inc();
    f.get().inc();
    EXPECT_EQ(f.get().value(), 2.0);
}

TEST(MetricFamily, LabelledCounter) {
    CounterFamily f("test_counter", "help", {"method", "status"});
    f.labels({"GET", "200"}).inc();
    f.labels({"GET", "200"}).inc();
    f.labels({"POST", "201"}).inc();

    EXPECT_EQ(f.labels({"GET", "200"}).value(), 2.0);
    EXPECT_EQ(f.labels({"POST", "201"}).value(), 1.0);
}

TEST(MetricFamily, ClearRemovesAllInstances) {
    GaugeFamily f("test_gauge", "help", {"id"});
    f.labels({"a"}).set(1.0);
    f.labels({"b"}).set(2.0);

    int count = 0;
    f.visit([&](auto&, double) { ++count; });
    EXPECT_EQ(count, 2);

    f.clear();

    count = 0;
    f.visit([&](auto&, double) { ++count; });
    EXPECT_EQ(count, 0);
}

TEST(MetricFamily, ClearThenRepopulate) {
    GaugeFamily f("test_gauge", "help", {"id"});
    f.labels({"a"}).set(10.0);
    f.labels({"b"}).set(20.0);
    f.clear();

    // Re-create with different labels
    f.labels({"c"}).set(30.0);

    int count = 0;
    double total = 0;
    f.visit([&](auto& labels, double val) {
        ++count;
        total += val;
        EXPECT_EQ(labels[0], "c");
    });
    EXPECT_EQ(count, 1);
    EXPECT_DOUBLE_EQ(total, 30.0);
}

TEST(MetricFamily, VisitIteratesAll) {
    CounterFamily f("test", "help", {"k"});
    f.labels({"a"}).inc(10);
    f.labels({"b"}).inc(20);

    double total = 0;
    f.visit([&](auto&, double val) { total += val; });
    EXPECT_DOUBLE_EQ(total, 30.0);
}

// -- PrometheusFormatter ------------------------------------------------------

TEST(PrometheusFormatter, CounterNoLabels) {
    CounterFamily f("test_total", "A test counter", {});
    f.get().inc(42);

    PrometheusFormatter fmt;
    std::string out;
    fmt.format(f, out);

    EXPECT_NE(out.find("# HELP test_total A test counter"), std::string::npos);
    EXPECT_NE(out.find("# TYPE test_total counter"), std::string::npos);
    EXPECT_NE(out.find("test_total 42"), std::string::npos);
}

TEST(PrometheusFormatter, GaugeWithLabels) {
    GaugeFamily f("temp", "Temperature", {"location"});
    f.labels({"kitchen"}).set(22.5);

    PrometheusFormatter fmt;
    std::string out;
    fmt.format(f, out);

    EXPECT_NE(out.find("# TYPE temp gauge"), std::string::npos);
    EXPECT_NE(out.find("temp{location=\"kitchen\"} 22.5"), std::string::npos);
}

TEST(PrometheusFormatter, LabelValueEscaping) {
    GaugeFamily f("test", "help", {"msg"});
    f.labels({"line1\nline2"}).set(1.0);
    f.labels({"say \"hello\""}).set(2.0);
    f.labels({"back\\slash"}).set(3.0);

    PrometheusFormatter fmt;
    std::string out;
    fmt.format(f, out);

    EXPECT_NE(out.find("msg=\"line1\\nline2\""), std::string::npos);
    EXPECT_NE(out.find("msg=\"say \\\"hello\\\"\""), std::string::npos);
    EXPECT_NE(out.find("msg=\"back\\\\slash\""), std::string::npos);
}

TEST(PrometheusFormatter, NegativeGaugeValue) {
    GaugeFamily f("test", "help", {});
    f.get().set(-42.0);

    PrometheusFormatter fmt;
    std::string out;
    fmt.format(f, out);

    EXPECT_NE(out.find("test -42"), std::string::npos);
}

TEST(PrometheusFormatter, ZeroValue) {
    CounterFamily f("test_total", "help", {});
    f.get(); // Create the label-less entry (stays at 0)

    PrometheusFormatter fmt;
    std::string out;
    fmt.format(f, out);

    EXPECT_NE(out.find("test_total 0"), std::string::npos);
}

TEST(PrometheusFormatter, MultipleLabels) {
    CounterFamily f("req", "help", {"method", "status"});
    f.labels({"GET", "200"}).inc(100);

    PrometheusFormatter fmt;
    std::string out;
    fmt.format(f, out);

    EXPECT_NE(out.find("req{method=\"GET\",status=\"200\"} 100"), std::string::npos);
}

// -- MetricsRegistry ----------------------------------------------------------

TEST(MetricsRegistry, CollectIncludesAllFamilies) {
    // Use a fresh registry (can't easily reset the singleton, so just verify
    // the singleton has content from file-scope statics)
    auto& reg = MetricsRegistry::instance();
    auto output = reg.collect();

    // Should contain at least the metrics registered by file-scope statics
    // in RestRouter.cpp, WebSocketServer.cpp, etc.
    EXPECT_FALSE(output.empty());
    EXPECT_NE(output.find("# TYPE"), std::string::npos);
}

TEST(MetricsRegistry, CollectorCallbackRuns) {
    auto& reg = MetricsRegistry::instance();
    auto& g = reg.gauge("test_collector_ran", "test", {});

    reg.add_collector([&g] { g.get().set(999.0); });

    auto output = reg.collect();
    EXPECT_NE(output.find("test_collector_ran 999"), std::string::npos);
}
