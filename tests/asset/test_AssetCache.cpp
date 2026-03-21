#include "asset/Asset.h"
#include "asset/AssetCache.h"

#include <gtest/gtest.h>
#include <memory>
#include <string>

// Minimal concrete Asset for testing.
struct FakeAsset : public Asset {
    FakeAsset(std::string path, size_t size) : Asset(std::move(path), "fake"), size(size) {
    }
    size_t memory_size() const override {
        return size;
    }
    size_t size;
};

static std::shared_ptr<FakeAsset> make_asset(const std::string& path, size_t size = 100) {
    return std::make_shared<FakeAsset>(path, size);
}

// ---------------------------------------------------------------------------
// Basic operations
// ---------------------------------------------------------------------------

TEST(AssetCache, MissOnEmptyCache) {
    AssetCache cache(1024);
    EXPECT_EQ(cache.get("nonexistent"), nullptr);
}

TEST(AssetCache, HitAfterInsert) {
    AssetCache cache(1024);
    auto asset = make_asset("textures/foo");
    cache.insert(asset);
    EXPECT_NE(cache.get("textures/foo"), nullptr);
}

TEST(AssetCache, GetReturnsSameSharedPtr) {
    AssetCache cache(1024);
    auto asset = make_asset("textures/foo");
    cache.insert(asset);
    EXPECT_EQ(cache.get("textures/foo"), asset);
}

TEST(AssetCache, UsedBytesAccountedCorrectly) {
    AssetCache cache(1024);
    cache.insert(make_asset("a", 300));
    EXPECT_EQ(cache.used_bytes(), 300u);
    cache.insert(make_asset("b", 200));
    EXPECT_EQ(cache.used_bytes(), 500u);
}

// ---------------------------------------------------------------------------
// Re-insertion replaces the entry
// ---------------------------------------------------------------------------

TEST(AssetCache, ReinsertReplacesEntry) {
    AssetCache cache(1024);
    cache.insert(make_asset("a", 100));
    auto newer = make_asset("a", 50);
    cache.insert(newer);
    EXPECT_EQ(cache.get("a"), newer);
    EXPECT_EQ(cache.used_bytes(), 50u);
}

// ---------------------------------------------------------------------------
// LRU eviction
// ---------------------------------------------------------------------------

TEST(AssetCache, EvictsLruWhenOverLimit) {
    // Limit of 200 bytes; insert three 100-byte assets.
    // After the third insert the oldest should be evicted.
    AssetCache cache(200);
    cache.insert(make_asset("a", 100));
    cache.insert(make_asset("b", 100));
    // "a" is now LRU. Inserting "c" (100) pushes total to 300 → evict "a".
    cache.insert(make_asset("c", 100));

    EXPECT_EQ(cache.get("a"), nullptr); // evicted
    EXPECT_NE(cache.get("b"), nullptr);
    EXPECT_NE(cache.get("c"), nullptr);
    EXPECT_LE(cache.used_bytes(), 200u);
}

TEST(AssetCache, PinnedAssetNotEvicted) {
    AssetCache cache(100);
    auto pinned = make_asset("pinned", 100);
    cache.insert(pinned);
    // Hold an external shared_ptr so use_count > 1 → cache won't evict it.
    cache.insert(make_asset("other", 100));
    EXPECT_NE(cache.get("pinned"), nullptr);
}

TEST(AssetCache, GetPromotesToMru) {
    // Insert a, b, c. Access "a" to promote it, then insert "d" to force eviction.
    // "b" should be evicted (oldest unpinned), not "a".
    AssetCache cache(300);
    cache.insert(make_asset("a", 100));
    cache.insert(make_asset("b", 100));
    cache.insert(make_asset("c", 100));
    cache.get("a");                     // promote "a" to MRU
    cache.insert(make_asset("d", 100)); // pushes to 400 → evict LRU="b"

    EXPECT_EQ(cache.get("b"), nullptr);
    EXPECT_NE(cache.get("a"), nullptr);
    EXPECT_NE(cache.get("c"), nullptr);
    EXPECT_NE(cache.get("d"), nullptr);
}

// ---------------------------------------------------------------------------
// Pressure-based eviction
// ---------------------------------------------------------------------------

TEST(AssetCache, EvictDoesNothingUnderBudget) {
    AssetCache cache(1024);
    cache.insert(make_asset("a", 100));
    cache.insert(make_asset("b", 100));
    // Under budget (200 < 1024) → evict() should keep everything.
    cache.evict();
    EXPECT_NE(cache.get("a"), nullptr);
    EXPECT_NE(cache.get("b"), nullptr);
    EXPECT_EQ(cache.used_bytes(), 200u);
}

TEST(AssetCache, EvictRemovesLruWhenOverBudget) {
    AssetCache cache(150);
    cache.insert(make_asset("a", 100));
    cache.insert(make_asset("b", 100));
    // Over budget (200 > 150) — "a" is LRU and unpinned, should be evicted.
    EXPECT_EQ(cache.get("a"), nullptr);
    EXPECT_NE(cache.get("b"), nullptr);
    EXPECT_EQ(cache.used_bytes(), 100u);
}

TEST(AssetCache, EvictSparesPinnedEvenOverBudget) {
    AssetCache cache(150);
    auto pinned = make_asset("pinned", 100);
    cache.insert(pinned);
    cache.insert(make_asset("unpinned", 100));
    // Over budget, but "pinned" is held externally → only "unpinned" evicted.
    // "unpinned" is MRU but also the only evictable entry.
    cache.evict();
    EXPECT_NE(cache.get("pinned"), nullptr);
    EXPECT_EQ(cache.used_bytes(), 100u);
}

TEST(AssetCache, UnusedEntriesStayWarmUnderBudget) {
    AssetCache cache(1024);
    cache.insert(make_asset("warm", 100));
    // Drop all external references — entry is unused but under budget.
    cache.evict();
    // Should still be retrievable from cache.
    EXPECT_NE(cache.get("warm"), nullptr);
}
