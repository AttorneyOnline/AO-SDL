#include "game/IPReputationService.h"

#include <gtest/gtest.h>

#include <chrono>
#include <cstdio>

namespace {

class IPReputationServiceTest : public ::testing::Test {
  protected:
    void SetUp() override {
        ReputationConfig cfg;
        cfg.enabled = true;
        cfg.cache_ttl_hours = 1;
        cfg.cache_failure_ttl_minutes = 1;
        cfg.ip_api_enabled = false; // Don't hit real APIs in tests
        svc_.configure(cfg);
    }

    IPReputationService svc_;
};

IPReputationEntry make_entry(const std::string& ip, uint32_t asn = 12345, bool proxy = false) {
    IPReputationEntry e;
    e.ip = ip;
    e.asn = asn;
    e.as_org = "TestOrg";
    e.country_code = "US";
    e.isp = "TestISP";
    e.is_proxy = proxy;
    e.fetched_at = std::chrono::duration_cast<std::chrono::seconds>(
                       std::chrono::system_clock::now().time_since_epoch())
                       .count();
    e.expires_at = e.fetched_at + 3600; // 1 hour
    return e;
}

} // namespace

TEST_F(IPReputationServiceTest, EmptyCacheReturnsNullopt) {
    EXPECT_FALSE(svc_.find_cached("1.2.3.4").has_value());
}

TEST_F(IPReputationServiceTest, PutAndFindCached) {
    svc_.put(make_entry("1.2.3.4"));
    auto result = svc_.find_cached("1.2.3.4");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->ip, "1.2.3.4");
    EXPECT_EQ(result->asn, 12345u);
    EXPECT_EQ(result->country_code, "US");
}

TEST_F(IPReputationServiceTest, CacheSizeTracksEntries) {
    EXPECT_EQ(svc_.cache_size(), 0u);
    svc_.put(make_entry("1.2.3.4"));
    EXPECT_EQ(svc_.cache_size(), 1u);
    svc_.put(make_entry("5.6.7.8"));
    EXPECT_EQ(svc_.cache_size(), 2u);
}

TEST_F(IPReputationServiceTest, PutOverwritesExisting) {
    svc_.put(make_entry("1.2.3.4", 111));
    svc_.put(make_entry("1.2.3.4", 222));
    auto result = svc_.find_cached("1.2.3.4");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->asn, 222u);
}

TEST_F(IPReputationServiceTest, ExpiredEntryNotReturned) {
    auto entry = make_entry("1.2.3.4");
    entry.expires_at = entry.fetched_at - 1; // Already expired
    svc_.put(entry);
    EXPECT_FALSE(svc_.find_cached("1.2.3.4").has_value());
}

TEST_F(IPReputationServiceTest, SweepRemovesExpired) {
    auto fresh = make_entry("1.2.3.4");
    auto stale = make_entry("5.6.7.8");
    stale.expires_at = stale.fetched_at - 1; // Already expired
    svc_.put(fresh);
    svc_.put(stale);

    EXPECT_EQ(svc_.cache_size(), 2u);
    size_t removed = svc_.sweep_expired();
    EXPECT_EQ(removed, 1u);
    EXPECT_EQ(svc_.cache_size(), 1u);
    EXPECT_TRUE(svc_.find_cached("1.2.3.4").has_value());
}

TEST_F(IPReputationServiceTest, LookupWithDisabledServiceReturnsNullopt) {
    ReputationConfig cfg;
    cfg.enabled = false;
    svc_.configure(cfg);

    svc_.put(make_entry("1.2.3.4"));
    // lookup() respects enabled flag; find_cached does not
    auto result = svc_.lookup("9.9.9.9");
    EXPECT_FALSE(result.has_value());
}

TEST_F(IPReputationServiceTest, LookupReturnsCacheHit) {
    svc_.put(make_entry("1.2.3.4"));
    auto result = svc_.lookup("1.2.3.4");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->ip, "1.2.3.4");
}

TEST_F(IPReputationServiceTest, ProxyFlagPreserved) {
    svc_.put(make_entry("1.2.3.4", 12345, true));
    auto result = svc_.find_cached("1.2.3.4");
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->is_proxy);
}

TEST_F(IPReputationServiceTest, PersistenceRoundTrip) {
    std::string path = "test_ip_rep_cache.json";

    svc_.put(make_entry("1.2.3.4", 111));
    svc_.put(make_entry("5.6.7.8", 222, true));
    svc_.save_sync(path);

    IPReputationService svc2;
    svc2.configure({.enabled = true, .ip_api_enabled = false});
    svc2.load(path);

    EXPECT_EQ(svc2.cache_size(), 2u);

    auto e1 = svc2.find_cached("1.2.3.4");
    ASSERT_TRUE(e1.has_value());
    EXPECT_EQ(e1->asn, 111u);
    EXPECT_FALSE(e1->is_proxy);

    auto e2 = svc2.find_cached("5.6.7.8");
    ASSERT_TRUE(e2.has_value());
    EXPECT_EQ(e2->asn, 222u);
    EXPECT_TRUE(e2->is_proxy);

    std::remove(path.c_str());
}

TEST_F(IPReputationServiceTest, LoadSkipsExpiredEntries) {
    std::string path = "test_ip_rep_expired.json";

    auto fresh = make_entry("1.2.3.4");
    auto stale = make_entry("5.6.7.8");
    stale.expires_at = stale.fetched_at - 100;
    svc_.put(fresh);
    svc_.put(stale);
    svc_.save_sync(path);

    IPReputationService svc2;
    svc2.configure({.enabled = true, .ip_api_enabled = false});
    svc2.load(path);

    EXPECT_EQ(svc2.cache_size(), 1u);
    EXPECT_TRUE(svc2.find_cached("1.2.3.4").has_value());
    EXPECT_FALSE(svc2.find_cached("5.6.7.8").has_value());

    std::remove(path.c_str());
}

TEST_F(IPReputationServiceTest, LoadNonexistentFileIsNoOp) {
    svc_.load("nonexistent_file_that_does_not_exist.json");
    EXPECT_EQ(svc_.cache_size(), 0u);
}

TEST_F(IPReputationServiceTest, AbuseIPDBBudgetStartsFull) {
    ReputationConfig cfg;
    cfg.enabled = true;
    cfg.abuseipdb_daily_budget = 500;
    svc_.configure(cfg);
    EXPECT_EQ(svc_.abuseipdb_budget_remaining(), 500);
}
