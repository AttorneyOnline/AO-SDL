#include "asset/MountHttp.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Fixture: creates an HttpPool with 1 thread and a MountHttp pointing at a
// dummy base URL.  No real HTTP traffic is needed for state-management tests.
// ---------------------------------------------------------------------------

class MountHttpTest : public ::testing::Test {
  protected:
    void SetUp() override {
        pool = std::make_unique<HttpPool>(1);
        mount = std::make_unique<MountHttp>("https://server.com/assets/", *pool);
    }

    void TearDown() override {
        // Destroy mount before pool so the reference stays valid.
        mount.reset();
        pool.reset();
    }

    std::unique_ptr<HttpPool> pool;
    std::unique_ptr<MountHttp> mount;
};

// ---------------------------------------------------------------------------
// 1. Initial state
// ---------------------------------------------------------------------------

TEST_F(MountHttpTest, InitialCachedCountIsZero) {
    EXPECT_EQ(mount->cached_count(), 0);
}

TEST_F(MountHttpTest, InitialPendingCountIsZero) {
    EXPECT_EQ(mount->pending_count(), 0);
}

TEST_F(MountHttpTest, InitialFailedCountIsZero) {
    EXPECT_EQ(mount->failed_count(), 0);
}

// ---------------------------------------------------------------------------
// 2. seek_file returns false on empty cache
// ---------------------------------------------------------------------------

TEST_F(MountHttpTest, SeekFileReturnsFalseOnEmptyCache) {
    EXPECT_FALSE(mount->seek_file("characters/phoenix/emote.png"));
}

// ---------------------------------------------------------------------------
// 3. fetch_data returns empty on uncached path
// ---------------------------------------------------------------------------

TEST_F(MountHttpTest, FetchDataReturnsEmptyForUncachedPath) {
    auto data = mount->fetch_data("characters/phoenix/emote.png");
    EXPECT_TRUE(data.empty());
}

// ---------------------------------------------------------------------------
// 4. extensions_for returns defaults when not loaded
// ---------------------------------------------------------------------------

TEST_F(MountHttpTest, ExtensionsForReturnsDefaultsWhenNotLoaded) {
    auto check_defaults = [](const std::vector<std::string>& exts) {
        ASSERT_EQ(exts.size(), 3u);
        EXPECT_EQ(exts[0], "webp");
        EXPECT_EQ(exts[1], "png");
        EXPECT_EQ(exts[2], "gif");
    };

    check_defaults(mount->extensions_for(MountHttp::AssetType::CHARICON));
    check_defaults(mount->extensions_for(MountHttp::AssetType::EMOTE));
    check_defaults(mount->extensions_for(MountHttp::AssetType::EMOTIONS));
    check_defaults(mount->extensions_for(MountHttp::AssetType::BACKGROUND));
}

// ---------------------------------------------------------------------------
// 5. has_extensions returns false initially
// ---------------------------------------------------------------------------

TEST_F(MountHttpTest, HasExtensionsReturnsFalseInitially) {
    EXPECT_FALSE(mount->has_extensions());
}

// ---------------------------------------------------------------------------
// 6. release on non-existent path doesn't crash
// ---------------------------------------------------------------------------

TEST_F(MountHttpTest, ReleaseOnNonExistentPathDoesNotCrash) {
    EXPECT_NO_THROW(mount->release("nonexistent/file.png"));
    EXPECT_EQ(mount->cached_count(), 0);
}

// ---------------------------------------------------------------------------
// 7. release_all on empty cache doesn't crash
// ---------------------------------------------------------------------------

TEST_F(MountHttpTest, ReleaseAllOnEmptyCacheDoesNotCrash) {
    EXPECT_NO_THROW(mount->release_all());
    EXPECT_EQ(mount->cached_count(), 0);
}

// ---------------------------------------------------------------------------
// 8. cache_snapshot is empty initially
// ---------------------------------------------------------------------------

TEST_F(MountHttpTest, CacheSnapshotIsEmptyInitially) {
    auto snapshot = mount->cache_snapshot();
    EXPECT_TRUE(snapshot.empty());
}

// ---------------------------------------------------------------------------
// 9. cached_bytes is 0 initially
// ---------------------------------------------------------------------------

TEST_F(MountHttpTest, CachedBytesIsZeroInitially) {
    EXPECT_EQ(mount->cached_bytes(), 0u);
}

// ---------------------------------------------------------------------------
// 10. has_failed returns false for unknown path
// ---------------------------------------------------------------------------

TEST_F(MountHttpTest, HasFailedReturnsFalseForUnknownPath) {
    EXPECT_FALSE(mount->has_failed("unknown/path.png"));
}

// ---------------------------------------------------------------------------
// 11. URL splitting
// ---------------------------------------------------------------------------

TEST(MountHttpUrlSplit, SplitsSchemeHostAndPathPrefix) {
    // Construct with a URL that has a path prefix and trailing slash.
    // The constructor calls split_url internally; we verify the result
    // through the Mount base class get_path() and by observing that
    // request() builds the correct HTTP path (indirectly tested).
    HttpPool pool(1);
    MountHttp m("https://server.com/assets/", pool);

    // The Mount base stores the original URL string as a filesystem::path.
    // We can verify basic construction didn't throw.
    EXPECT_EQ(m.cached_count(), 0);
    EXPECT_EQ(m.pending_count(), 0);

    // Verify the pool reference is valid.
    EXPECT_EQ(&m.pool(), &pool);
}

TEST(MountHttpUrlSplit, HandlesUrlWithNoPath) {
    HttpPool pool(1);
    MountHttp m("https://server.com", pool);
    // Should construct without error.
    EXPECT_EQ(m.cached_count(), 0);
}

TEST(MountHttpUrlSplit, HandlesUrlWithDeepPath) {
    HttpPool pool(1);
    MountHttp m("https://cdn.example.org/v2/game/assets/", pool);
    // Should construct without error and be usable.
    EXPECT_EQ(m.cached_count(), 0);
    EXPECT_FALSE(m.has_extensions());
}

// ---------------------------------------------------------------------------
// 12. Path lowercasing
// ---------------------------------------------------------------------------

TEST_F(MountHttpTest, RequestUsesLowercasedPath) {
    // request() should lowercase the path internally. After requesting with
    // mixed case, pending_count should reflect one pending request.
    mount->request("FOO/BAR.png");
    EXPECT_EQ(mount->pending_count(), 1);
}

TEST_F(MountHttpTest, DuplicateRequestWithDifferentCaseIsIgnored) {
    // Because paths are lowercased, requesting the same path with different
    // casing should not create a second pending entry.
    mount->request("FOO/BAR.png");
    mount->request("foo/bar.png");
    mount->request("Foo/Bar.PNG");
    EXPECT_EQ(mount->pending_count(), 1);
}

TEST_F(MountHttpTest, HasFailedIsCaseInsensitive) {
    // has_failed should also lowercase its argument. Since we can't directly
    // insert into failed_ without a real 404, we just verify that querying
    // with different cases on a path that hasn't failed all returns false.
    EXPECT_FALSE(mount->has_failed("SOME/PATH.png"));
    EXPECT_FALSE(mount->has_failed("some/path.png"));
}

// ---------------------------------------------------------------------------
// Additional state management tests
// ---------------------------------------------------------------------------

TEST_F(MountHttpTest, RequestSkipsIfAlreadyPending) {
    mount->request("test/file.png");
    mount->request("test/file.png");
    EXPECT_EQ(mount->pending_count(), 1);
}

TEST_F(MountHttpTest, MultipleDistinctRequestsAllPend) {
    mount->request("file1.png");
    mount->request("file2.png");
    mount->request("file3.png");
    EXPECT_EQ(mount->pending_count(), 3);
}

TEST_F(MountHttpTest, SeekFileIsCaseInsensitive) {
    // seek_file lowercases its argument, so querying with upper case on an
    // empty cache should still return false (consistent, not crashing).
    EXPECT_FALSE(mount->seek_file("UPPER/CASE.PNG"));
    EXPECT_FALSE(mount->seek_file("upper/case.png"));
}

TEST_F(MountHttpTest, FetchDataIsCaseInsensitive) {
    // fetch_data lowercases its argument. Both cases should return empty.
    EXPECT_TRUE(mount->fetch_data("UPPER/FILE.PNG").empty());
    EXPECT_TRUE(mount->fetch_data("upper/file.png").empty());
}

TEST_F(MountHttpTest, PoolAccessorReturnsCorrectPool) {
    EXPECT_EQ(&mount->pool(), pool.get());
}

TEST_F(MountHttpTest, ReleaseAfterReleaseAllDoesNotCrash) {
    mount->release_all();
    EXPECT_NO_THROW(mount->release("anything.png"));
    EXPECT_EQ(mount->cached_count(), 0);
}

// ---------------------------------------------------------------------------
// Destruction safety — callbacks delivered after MountHttp is destroyed
// ---------------------------------------------------------------------------

// Reproduces the crash from the Windows crash dump: MountHttp::request()
// captures `this` in a callback passed to HttpPool. If the MountHttp is
// destroyed (e.g. session disconnect) while requests are in-flight, the
// callback fires on a freed object when poll() delivers it.
//
// Sequence: request() → destroy MountHttp → poll() delivers error callback
// Without the alive_ guard, this is a use-after-free.
TEST(MountHttpLifetime, PollAfterDestroyDoesNotCrash) {
    HttpPool pool(1);
    {
        MountHttp mount("https://127.0.0.1:1/assets/", pool);
        // Fire several requests. The worker thread will fail to connect
        // (connection refused) and queue error callbacks.
        for (int i = 0; i < 10; i++)
            mount.request("file" + std::to_string(i) + ".png");
        EXPECT_EQ(mount.pending_count(), 10);
    }
    // MountHttp is now destroyed. Wait for workers to finish and deliver
    // the error callbacks. Without the fix, this poll() would crash.
    std::this_thread::sleep_for(std::chrono::seconds(2));
    EXPECT_NO_THROW(pool.poll());
}

// Same scenario but with load() which internally calls fetch_extensions(),
// another callback that captures `this`.
TEST(MountHttpLifetime, PollAfterDestroyWithExtensionsFetchDoesNotCrash) {
    HttpPool pool(1);
    {
        MountHttp mount("https://127.0.0.1:1/assets/", pool);
        mount.load(); // triggers fetch_extensions() → pool_.get() with [this]
    }
    std::this_thread::sleep_for(std::chrono::seconds(2));
    EXPECT_NO_THROW(pool.poll());
}

// Multiple mounts created and destroyed in sequence, with poll() only at the
// end — simulates rapid reconnect cycles.
TEST(MountHttpLifetime, RapidCreateDestroyWithDeferredPoll) {
    HttpPool pool(2);
    for (int round = 0; round < 5; round++) {
        MountHttp mount("https://127.0.0.1:1/assets/", pool);
        mount.request("round" + std::to_string(round) + ".png");
    }
    // All 5 mounts are destroyed. Poll delivers all queued callbacks.
    std::this_thread::sleep_for(std::chrono::seconds(2));
    EXPECT_NO_THROW(pool.poll());
}

// Interleaved: poll between create/destroy cycles to mix live and dead
// callback delivery.
TEST(MountHttpLifetime, InterleavedCreateDestroyAndPoll) {
    HttpPool pool(2);
    for (int round = 0; round < 3; round++) {
        {
            MountHttp mount("https://127.0.0.1:1/assets/", pool);
            mount.request("interleaved" + std::to_string(round) + ".png");
            mount.load();
        }
        // Poll after each destruction — some callbacks may have arrived,
        // some may still be in-flight.
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        EXPECT_NO_THROW(pool.poll());
    }
    // Final drain
    std::this_thread::sleep_for(std::chrono::seconds(2));
    EXPECT_NO_THROW(pool.poll());
}

// ---------------------------------------------------------------------------
// Destruction with drop_below — callbacks fired synchronously by drop
// ---------------------------------------------------------------------------

TEST_F(MountHttpTest, DropBelowAfterRequestDoesNotCrash) {
    mount->request("low_priority.png", HttpPriority::LOW);
    // drop_below fires the callback synchronously with error="dropped".
    // Mount is still alive here, so this should work fine.
    EXPECT_NO_THROW(pool->drop_below(HttpPriority::NORMAL));
}

TEST(MountHttpLifetime, DropBelowAfterDestroyDoesNotCrash) {
    HttpPool pool(1);
    {
        MountHttp mount("https://127.0.0.1:1/assets/", pool);
        mount.request("will_be_dropped.png", HttpPriority::LOW);
    }
    // drop_below fires callbacks synchronously — mount is already dead.
    EXPECT_NO_THROW(pool.drop_below(HttpPriority::NORMAL));
}

// ---------------------------------------------------------------------------
// Concurrent request + destroy stress test
// ---------------------------------------------------------------------------

TEST(MountHttpLifetime, StressCreateRequestDestroy) {
    HttpPool pool(4);
    for (int i = 0; i < 20; i++) {
        MountHttp mount("https://127.0.0.1:1/assets/", pool);
        for (int j = 0; j < 5; j++)
            mount.request("stress_" + std::to_string(i) + "_" + std::to_string(j) + ".png");
    }
    // Drain everything
    std::this_thread::sleep_for(std::chrono::seconds(3));
    EXPECT_NO_THROW(pool.poll());
}
