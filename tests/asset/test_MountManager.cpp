#include "asset/MountManager.h"

#include "asset/MountEmbedded.h"

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Stub Mount: in-memory mount for testing priority and add_mount().
// ---------------------------------------------------------------------------

class StubMount : public Mount {
  public:
    explicit StubMount(const std::string& name = "stub://") : Mount(name) {
    }

    void add_file(const std::string& path, const std::vector<uint8_t>& data) {
        files_[path] = data;
    }

    void add_file(const std::string& path, const std::string& content) {
        files_[path] = {content.begin(), content.end()};
    }

    void load() override {
    }

    bool seek_file(const std::string& path) const override {
        return files_.find(path) != files_.end();
    }

    std::vector<uint8_t> fetch_data(const std::string& path) override {
        auto it = files_.find(path);
        if (it == files_.end())
            return {};
        return it->second;
    }

  protected:
    void load_cache() override {
    }
    void save_cache() override {
    }

  private:
    std::unordered_map<std::string, std::vector<uint8_t>> files_;
};

// ===========================================================================
// MountEmbedded tests
// ===========================================================================

class MountEmbeddedTest : public ::testing::Test {
  protected:
    void SetUp() override {
        mount.load();
    }

    MountEmbedded mount;
};

// ---------------------------------------------------------------------------
// 8. seek_file for known embedded assets
// ---------------------------------------------------------------------------

TEST_F(MountEmbeddedTest, SeekFileFindsVertexShader) {
    EXPECT_TRUE(mount.seek_file("shaders/text/glsl/vertex.glsl"));
}

TEST_F(MountEmbeddedTest, SeekFileFindsFragmentShader) {
    EXPECT_TRUE(mount.seek_file("shaders/text/glsl/fragment.glsl"));
}

TEST_F(MountEmbeddedTest, SeekFileFindsMainVertexShader) {
    EXPECT_TRUE(mount.seek_file("shaders/main/glsl/vertex.glsl"));
}

TEST_F(MountEmbeddedTest, SeekFileFindsMainFragmentShader) {
    EXPECT_TRUE(mount.seek_file("shaders/main/glsl/fragment.glsl"));
}

TEST_F(MountEmbeddedTest, SeekFileFindsThemeConfig) {
    EXPECT_TRUE(mount.seek_file("themes/default/courtroom_design.ini"));
}

TEST_F(MountEmbeddedTest, SeekFileFindsChatConfig) {
    EXPECT_TRUE(mount.seek_file("themes/default/chat_config.ini"));
}

TEST_F(MountEmbeddedTest, SeekFileFindsThemeChatImage) {
    EXPECT_TRUE(mount.seek_file("themes/default/chat.png"));
}

// ---------------------------------------------------------------------------
// 9. seek_file returns false for unknown paths
// ---------------------------------------------------------------------------

TEST_F(MountEmbeddedTest, SeekFileReturnsFalseForUnknownPath) {
    EXPECT_FALSE(mount.seek_file("does/not/exist.txt"));
}

TEST_F(MountEmbeddedTest, SeekFileReturnsFalseForEmptyPath) {
    EXPECT_FALSE(mount.seek_file(""));
}

TEST_F(MountEmbeddedTest, SeekFileReturnsFalseForPartialMatch) {
    EXPECT_FALSE(mount.seek_file("shaders/text/glsl/"));
}

TEST_F(MountEmbeddedTest, SeekFileReturnsFalseForDirectory) {
    EXPECT_FALSE(mount.seek_file("shaders/"));
}

// ---------------------------------------------------------------------------
// 10. fetch_data returns actual shader content
// ---------------------------------------------------------------------------

TEST_F(MountEmbeddedTest, FetchDataReturnsShaderContent) {
    auto data = mount.fetch_data("shaders/text/glsl/vertex.glsl");
    ASSERT_FALSE(data.empty());

    std::string content(data.begin(), data.end());
    // The vertex shader starts with a GLSL version directive
    EXPECT_NE(content.find("#version"), std::string::npos);
}

TEST_F(MountEmbeddedTest, FetchDataReturnsFragmentShaderContent) {
    auto data = mount.fetch_data("shaders/text/glsl/fragment.glsl");
    ASSERT_FALSE(data.empty());

    std::string content(data.begin(), data.end());
    EXPECT_NE(content.find("#version"), std::string::npos);
}

TEST_F(MountEmbeddedTest, FetchDataReturnsIniContent) {
    auto data = mount.fetch_data("themes/default/courtroom_design.ini");
    ASSERT_FALSE(data.empty());

    std::string content(data.begin(), data.end());
    // INI files typically contain section headers or key=value pairs
    EXPECT_FALSE(content.empty());
}

TEST_F(MountEmbeddedTest, FetchDataReturnsEmptyForUnknownPath) {
    auto data = mount.fetch_data("nonexistent/file.txt");
    EXPECT_TRUE(data.empty());
}

TEST_F(MountEmbeddedTest, FetchDataReturnsPngContent) {
    auto data = mount.fetch_data("themes/default/chat.png");
    ASSERT_GE(data.size(), 8u);

    // PNG magic bytes: 0x89 0x50 0x4E 0x47 0x0D 0x0A 0x1A 0x0A
    EXPECT_EQ(data[0], 0x89);
    EXPECT_EQ(data[1], 0x50); // 'P'
    EXPECT_EQ(data[2], 0x4E); // 'N'
    EXPECT_EQ(data[3], 0x47); // 'G'
}

TEST_F(MountEmbeddedTest, LoadIsIdempotent) {
    // Loading again should not corrupt existing data
    mount.load();

    EXPECT_TRUE(mount.seek_file("shaders/text/glsl/vertex.glsl"));
    auto data = mount.fetch_data("shaders/text/glsl/vertex.glsl");
    ASSERT_FALSE(data.empty());

    std::string content(data.begin(), data.end());
    EXPECT_NE(content.find("#version"), std::string::npos);
}

// ===========================================================================
// MountManager tests
// ===========================================================================

class MountManagerTest : public ::testing::Test {
  protected:
    MountManager manager;
};

// ---------------------------------------------------------------------------
// 1. Default construction (has embedded mount)
// ---------------------------------------------------------------------------

TEST_F(MountManagerTest, DefaultConstructionHasEmbeddedMount) {
    // A freshly constructed MountManager should find embedded assets
    auto data = manager.fetch_data("shaders/text/glsl/vertex.glsl");
    ASSERT_TRUE(data.has_value());
    EXPECT_FALSE(data->empty());
}

TEST_F(MountManagerTest, DefaultConstructionFindsThemeFiles) {
    auto data = manager.fetch_data("themes/default/courtroom_design.ini");
    ASSERT_TRUE(data.has_value());
    EXPECT_FALSE(data->empty());
}

// ---------------------------------------------------------------------------
// 2. fetch_data for embedded assets (shaders, theme files)
// ---------------------------------------------------------------------------

TEST_F(MountManagerTest, FetchDataReturnsShaderContent) {
    auto data = manager.fetch_data("shaders/text/glsl/vertex.glsl");
    ASSERT_TRUE(data.has_value());

    std::string content(data->begin(), data->end());
    EXPECT_NE(content.find("#version"), std::string::npos);
}

TEST_F(MountManagerTest, FetchDataReturnsFragmentShader) {
    auto data = manager.fetch_data("shaders/main/glsl/fragment.glsl");
    ASSERT_TRUE(data.has_value());

    std::string content(data->begin(), data->end());
    EXPECT_NE(content.find("#version"), std::string::npos);
}

TEST_F(MountManagerTest, FetchDataReturnsNulloptForNonexistent) {
    auto data = manager.fetch_data("does/not/exist.txt");
    EXPECT_FALSE(data.has_value());
}

TEST_F(MountManagerTest, FetchDataReturnsNulloptForEmptyPath) {
    auto data = manager.fetch_data("");
    EXPECT_FALSE(data.has_value());
}

TEST_F(MountManagerTest, FetchDataReturnsChatPng) {
    auto data = manager.fetch_data("themes/default/chat.png");
    ASSERT_TRUE(data.has_value());
    ASSERT_GE(data->size(), 8u);

    // Verify PNG magic
    EXPECT_EQ((*data)[0], 0x89);
    EXPECT_EQ((*data)[1], 0x50);
}

// ---------------------------------------------------------------------------
// 3. add_mount() adds a new mount
// ---------------------------------------------------------------------------

TEST_F(MountManagerTest, AddMountMakesFilesAvailable) {
    auto stub = std::make_unique<StubMount>();
    stub->add_file("test/file.txt", "test content");

    manager.add_mount(std::move(stub));

    auto data = manager.fetch_data("test/file.txt");
    ASSERT_TRUE(data.has_value());

    std::string content(data->begin(), data->end());
    EXPECT_EQ(content, "test content");
}

TEST_F(MountManagerTest, AddMountDoesNotRemoveEmbedded) {
    auto stub = std::make_unique<StubMount>();
    stub->add_file("extra/data.bin", "binary data");

    manager.add_mount(std::move(stub));

    // Embedded assets should still be available
    auto shader = manager.fetch_data("shaders/text/glsl/vertex.glsl");
    ASSERT_TRUE(shader.has_value());
    EXPECT_FALSE(shader->empty());

    // And the newly added file too
    auto extra = manager.fetch_data("extra/data.bin");
    ASSERT_TRUE(extra.has_value());
}

TEST_F(MountManagerTest, AddMultipleMounts) {
    auto stub1 = std::make_unique<StubMount>("stub1://");
    stub1->add_file("from/mount1.txt", "mount1");

    auto stub2 = std::make_unique<StubMount>("stub2://");
    stub2->add_file("from/mount2.txt", "mount2");

    manager.add_mount(std::move(stub1));
    manager.add_mount(std::move(stub2));

    auto d1 = manager.fetch_data("from/mount1.txt");
    auto d2 = manager.fetch_data("from/mount2.txt");
    ASSERT_TRUE(d1.has_value());
    ASSERT_TRUE(d2.has_value());

    EXPECT_EQ(std::string(d1->begin(), d1->end()), "mount1");
    EXPECT_EQ(std::string(d2->begin(), d2->end()), "mount2");
}

// ---------------------------------------------------------------------------
// 4. load_mounts() with valid directory paths
// ---------------------------------------------------------------------------

class MountManagerLoadTest : public ::testing::Test {
  protected:
    fs::path temp_dir;

    void SetUp() override {
        temp_dir = fs::temp_directory_path() / "aosdl_test_mount_manager";
        fs::remove_all(temp_dir);
        fs::create_directories(temp_dir);

        write_file(temp_dir / "local_asset.txt", "local data");
        write_file(temp_dir / "override.txt", "local version");
    }

    void TearDown() override {
        fs::remove_all(temp_dir);
    }

    static void write_file(const fs::path& path, const std::string& content) {
        std::ofstream out(path, std::ios::binary);
        out.write(content.data(), static_cast<std::streamsize>(content.size()));
    }
};

TEST_F(MountManagerLoadTest, LoadMountsWithValidDirectory) {
    MountManager manager;
    manager.load_mounts({temp_dir});

    auto data = manager.fetch_data("local_asset.txt");
    ASSERT_TRUE(data.has_value());

    std::string content(data->begin(), data->end());
    EXPECT_EQ(content, "local data");
}

TEST_F(MountManagerLoadTest, LoadMountsPreservesEmbedded) {
    MountManager manager;
    manager.load_mounts({temp_dir});

    // Embedded assets should still be available after load_mounts
    auto shader = manager.fetch_data("shaders/text/glsl/vertex.glsl");
    ASSERT_TRUE(shader.has_value());
    EXPECT_FALSE(shader->empty());
}

TEST_F(MountManagerLoadTest, LoadMountsReplacesOldMounts) {
    MountManager manager;
    manager.load_mounts({temp_dir});

    // File from temp_dir should be available
    auto data1 = manager.fetch_data("local_asset.txt");
    ASSERT_TRUE(data1.has_value());

    // Create a different directory
    fs::path temp_dir2 = fs::temp_directory_path() / "aosdl_test_mount_manager2";
    fs::remove_all(temp_dir2);
    fs::create_directories(temp_dir2);
    write_file(temp_dir2 / "other_asset.txt", "other data");

    // load_mounts replaces mounts — old file should be gone
    manager.load_mounts({temp_dir2});

    auto old_data = manager.fetch_data("local_asset.txt");
    EXPECT_FALSE(old_data.has_value());

    auto new_data = manager.fetch_data("other_asset.txt");
    ASSERT_TRUE(new_data.has_value());

    std::string content(new_data->begin(), new_data->end());
    EXPECT_EQ(content, "other data");

    fs::remove_all(temp_dir2);
}

TEST_F(MountManagerLoadTest, LoadMountsWithMultipleDirectories) {
    fs::path temp_dir2 = fs::temp_directory_path() / "aosdl_test_mount_manager_multi";
    fs::remove_all(temp_dir2);
    fs::create_directories(temp_dir2);
    write_file(temp_dir2 / "second_file.txt", "from second");

    MountManager manager;
    manager.load_mounts({temp_dir, temp_dir2});

    auto d1 = manager.fetch_data("local_asset.txt");
    auto d2 = manager.fetch_data("second_file.txt");

    ASSERT_TRUE(d1.has_value());
    ASSERT_TRUE(d2.has_value());

    EXPECT_EQ(std::string(d1->begin(), d1->end()), "local data");
    EXPECT_EQ(std::string(d2->begin(), d2->end()), "from second");

    fs::remove_all(temp_dir2);
}

// ---------------------------------------------------------------------------
// 5. load_mounts() with invalid paths (skips gracefully)
// ---------------------------------------------------------------------------

TEST_F(MountManagerLoadTest, LoadMountsSkipsInvalidPaths) {
    fs::path bad_path = temp_dir / "nonexistent_subdir";

    MountManager manager;
    // Should not throw — invalid paths are skipped with a warning
    EXPECT_NO_THROW(manager.load_mounts({bad_path}));
}

TEST_F(MountManagerLoadTest, LoadMountsMixedValidAndInvalidPaths) {
    fs::path bad_path = temp_dir / "nonexistent_subdir";

    MountManager manager;
    EXPECT_NO_THROW(manager.load_mounts({bad_path, temp_dir}));

    // The valid mount should still work
    auto data = manager.fetch_data("local_asset.txt");
    ASSERT_TRUE(data.has_value());

    std::string content(data->begin(), data->end());
    EXPECT_EQ(content, "local data");
}

TEST_F(MountManagerLoadTest, LoadMountsWithEmptyList) {
    MountManager manager;
    EXPECT_NO_THROW(manager.load_mounts({}));

    // Only embedded assets should be available
    auto shader = manager.fetch_data("shaders/text/glsl/vertex.glsl");
    ASSERT_TRUE(shader.has_value());

    auto local = manager.fetch_data("local_asset.txt");
    EXPECT_FALSE(local.has_value());
}

// ---------------------------------------------------------------------------
// 6. prefetch() behavior
// ---------------------------------------------------------------------------

TEST_F(MountManagerTest, PrefetchDoesNotCrashWithNoHttpMounts) {
    // With only embedded mount, prefetch should be a no-op
    EXPECT_NO_THROW(manager.prefetch("characters/phoenix/normal.png"));
}

TEST_F(MountManagerTest, PrefetchWithLocalFileDoesNothing) {
    // If a local mount has the file, prefetch should skip HTTP
    auto stub = std::make_unique<StubMount>();
    stub->add_file("characters/phoenix/normal.png", "local sprite");

    // We need to insert before the embedded mount to test priority properly
    // but add_mount appends, so the stub will be checked after embedded
    manager.add_mount(std::move(stub));

    // Should not throw — local file exists, no HTTP needed
    EXPECT_NO_THROW(manager.prefetch("characters/phoenix/normal.png"));
}

TEST_F(MountManagerTest, PrefetchWithDefaultPriority) {
    EXPECT_NO_THROW(manager.prefetch("some/missing/asset.png", 1));
}

TEST_F(MountManagerTest, PrefetchWithHighPriority) {
    EXPECT_NO_THROW(manager.prefetch("some/missing/asset.png", 2));
}

// ---------------------------------------------------------------------------
// 7. Mount priority (local mounts searched before later mounts)
// ---------------------------------------------------------------------------

TEST_F(MountManagerTest, EmbeddedMountHasPriorityOverLaterAdded) {
    // Embedded mount is loaded at construction. If we add a mount that
    // has the same path, the embedded version (first in list) wins.
    auto stub = std::make_unique<StubMount>();
    stub->add_file("shaders/text/glsl/vertex.glsl", "fake shader");

    manager.add_mount(std::move(stub));

    auto data = manager.fetch_data("shaders/text/glsl/vertex.glsl");
    ASSERT_TRUE(data.has_value());

    // Should get the real embedded shader, not the fake one
    std::string content(data->begin(), data->end());
    EXPECT_NE(content, "fake shader");
    EXPECT_NE(content.find("#version"), std::string::npos);
}

TEST_F(MountManagerLoadTest, DiskMountHasPriorityOverEmbedded) {
    // Create a local file that shadows an embedded asset path
    fs::create_directories(temp_dir / "shaders" / "text" / "glsl");
    write_file(temp_dir / "shaders" / "text" / "glsl" / "vertex.glsl", "// local override shader");

    MountManager manager;
    manager.load_mounts({temp_dir});

    auto data = manager.fetch_data("shaders/text/glsl/vertex.glsl");
    ASSERT_TRUE(data.has_value());

    // Disk mount is checked before embedded, so local version wins
    std::string content(data->begin(), data->end());
    EXPECT_EQ(content, "// local override shader");
}

TEST_F(MountManagerLoadTest, FirstDiskMountHasPriorityOverSecond) {
    fs::path temp_dir2 = fs::temp_directory_path() / "aosdl_test_mount_manager_prio";
    fs::remove_all(temp_dir2);
    fs::create_directories(temp_dir2);
    write_file(temp_dir2 / "override.txt", "second version");

    MountManager manager;
    manager.load_mounts({temp_dir, temp_dir2});

    // Both dirs have "override.txt" — first directory wins
    auto data = manager.fetch_data("override.txt");
    ASSERT_TRUE(data.has_value());

    std::string content(data->begin(), data->end());
    EXPECT_EQ(content, "local version");

    fs::remove_all(temp_dir2);
}

// ---------------------------------------------------------------------------
// http_extensions() — no HTTP mounts means empty results
// ---------------------------------------------------------------------------

TEST_F(MountManagerTest, HttpExtensionsReturnsEmptyWithNoHttpMount) {
    auto exts = manager.http_extensions(0);
    EXPECT_TRUE(exts.empty());
}

TEST_F(MountManagerTest, HttpExtensionsReturnsEmptyForAllTypes) {
    // Test each AssetType enum value (0..3)
    for (int type = 0; type < 4; ++type) {
        auto exts = manager.http_extensions(type);
        EXPECT_TRUE(exts.empty());
    }
}

// ---------------------------------------------------------------------------
// http_stats() — no HTTP mounts means zeroed stats
// ---------------------------------------------------------------------------

TEST_F(MountManagerTest, HttpStatsAllZerosWithNoHttpMount) {
    auto stats = manager.http_stats();
    EXPECT_EQ(stats.pending, 0);
    EXPECT_EQ(stats.cached, 0);
    EXPECT_EQ(stats.failed, 0);
    EXPECT_EQ(stats.pool_pending, 0);
    EXPECT_EQ(stats.cached_bytes, 0u);
}

// ---------------------------------------------------------------------------
// Additional http-related methods with no HTTP mounts
// ---------------------------------------------------------------------------

TEST_F(MountManagerTest, HttpCacheSnapshotEmptyWithNoHttpMount) {
    auto snapshot = manager.http_cache_snapshot();
    EXPECT_TRUE(snapshot.empty());
}

TEST_F(MountManagerTest, ReleaseAllHttpDoesNotCrashWithNoHttpMount) {
    EXPECT_NO_THROW(manager.release_all_http());
}

TEST_F(MountManagerTest, ReleaseHttpDoesNotCrashWithNoHttpMount) {
    EXPECT_NO_THROW(manager.release_http("some/path.png"));
}

TEST_F(MountManagerTest, DropHttpBelowDoesNotCrashWithNoHttpMount) {
    EXPECT_NO_THROW(manager.drop_http_below(1));
}

TEST_F(MountManagerTest, FetchStreamingReturnsFalseWithNoHttpMount) {
    bool result = manager.fetch_streaming("some/path.png", [](const uint8_t*, size_t) { return true; });
    EXPECT_FALSE(result);
}
