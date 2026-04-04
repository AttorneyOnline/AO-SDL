/**
 * @file PrometheusFormatter.h
 * @brief Renders metrics in Prometheus text exposition format (v0.0.4).
 *
 * Output is suitable for serving on a /metrics HTTP endpoint that
 * Prometheus scrapes at regular intervals.
 */
#pragma once

#include "metrics/Metrics.h"

#include <cmath>
#include <sstream>
#include <string>

namespace metrics {

class PrometheusFormatter {
  public:
    /// Render a single metric family to Prometheus text format.
    /// Writes directly to the output buffer — no intermediate allocations.
    void format(const MetricFamilyBase& family, std::string& out) const {
        out += "# HELP ";
        out += family.name();
        out += ' ';
        out += family.help();
        out += '\n';
        out += "# TYPE ";
        out += family.name();
        out += ' ';
        out += family.type_string();
        out += '\n';

        auto& label_names = family.label_names();

        family.visit([&](const std::vector<std::string>& label_values, double value) {
            out += family.name();

            if (!label_names.empty() && !label_values.empty()) {
                out += '{';
                for (size_t i = 0; i < label_names.size() && i < label_values.size(); ++i) {
                    if (i > 0)
                        out += ',';
                    out += label_names[i];
                    out += "=\"";
                    append_escaped(out, label_values[i]);
                    out += '"';
                }
                out += '}';
            }

            out += ' ';
            append_value(out, value);
            out += '\n';
        });

        out += '\n';
    }

  private:
    /// Escape and append directly to output — no temporary string.
    static void append_escaped(std::string& out, const std::string& s) {
        for (char c : s) {
            if (c == '\\')
                out += "\\\\";
            else if (c == '"')
                out += "\\\"";
            else if (c == '\n')
                out += "\\n";
            else
                out += c;
        }
    }

    /// Format a double directly into the output buffer.
    static void append_value(std::string& out, double v) {
        if (std::isinf(v)) {
            out += v > 0 ? "+Inf" : "-Inf";
            return;
        }
        if (std::isnan(v)) {
            out += "NaN";
            return;
        }
        // Integer fast path — avoids ostringstream
        if (v >= 0 && v == static_cast<double>(static_cast<uint64_t>(v)) && v < 1e15) {
            char buf[24];
            auto len = std::snprintf(buf, sizeof(buf), "%llu", static_cast<unsigned long long>(v));
            out.append(buf, static_cast<size_t>(len));
            return;
        }
        if (v < 0 && v == static_cast<double>(static_cast<int64_t>(v)) && v > -1e15) {
            char buf[24];
            auto len = std::snprintf(buf, sizeof(buf), "%lld", static_cast<long long>(v));
            out.append(buf, static_cast<size_t>(len));
            return;
        }
        char buf[32];
        auto len = std::snprintf(buf, sizeof(buf), "%g", v);
        out.append(buf, static_cast<size_t>(len));
    }
};

} // namespace metrics
