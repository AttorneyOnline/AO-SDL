#include "asset/MountDirectory.h"

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Helper: creates a temporary directory with known files for testing.
// Cleans up on destruction.
// ---------------------------------------------------------------------------

class MountDirectoryTest : public ::testing::Test {
  protected:
    fs::path temp_dir;

    void SetUp() override {
        temp_dir = fs::temp_directory_path() / "aosdl_test_mount_directory";
        fs::remove_all(temp_dir);
        fs::create_directories(temp_dir);
        fs::create_directories(temp_dir / "subdir");

        write_file(temp_dir / "hello.txt", "Hello, world!");
        write_file(temp_dir / "binary.bin", std::string("\x00\x01\x02\x03", 4));
        write_file(temp_dir / "subdir" / "nested.txt", "nested content");
    }

    void TearDown() override {
        fs::remove_all(temp_dir);
    }

    static void write_file(const fs::path& path, const std::string& content) {
        std::ofstream out(path, std::ios::binary);
        out.write(content.data(), static_cast<std::streamsize>(content.size()));
    }
};

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

TEST_F(MountDirectoryTest, ConstructionWithValidDirectory) {
    MountDirectory mount(temp_dir);
    EXPECT_EQ(mount.get_path(), temp_dir);
    EXPECT_NO_THROW(mount.load());
}

TEST_F(MountDirectoryTest, ConstructionWithNonexistentPath) {
    fs::path bad_path = temp_dir / "does_not_exist";
    MountDirectory mount(bad_path);
    // load() should throw because the path is not a directory.
    EXPECT_THROW(mount.load(), std::runtime_error);
}

TEST_F(MountDirectoryTest, ConstructionWithFilePath) {
    fs::path file_path = temp_dir / "hello.txt";
    MountDirectory mount(file_path);
    // A file is not a directory — load() should throw.
    EXPECT_THROW(mount.load(), std::runtime_error);
}

// ---------------------------------------------------------------------------
// seek_file
// ---------------------------------------------------------------------------

TEST_F(MountDirectoryTest, SeekFileReturnsTrueForExistingFile) {
    MountDirectory mount(temp_dir);
    mount.load();
    EXPECT_TRUE(mount.seek_file("hello.txt"));
}

TEST_F(MountDirectoryTest, SeekFileReturnsTrueForNestedFile) {
    MountDirectory mount(temp_dir);
    mount.load();
    EXPECT_TRUE(mount.seek_file("subdir/nested.txt"));
}

TEST_F(MountDirectoryTest, SeekFileReturnsFalseForNonexistentFile) {
    MountDirectory mount(temp_dir);
    mount.load();
    EXPECT_FALSE(mount.seek_file("no_such_file.txt"));
}

TEST_F(MountDirectoryTest, SeekFileReturnsFalseForNonexistentNestedFile) {
    MountDirectory mount(temp_dir);
    mount.load();
    EXPECT_FALSE(mount.seek_file("subdir/missing.txt"));
}

// ---------------------------------------------------------------------------
// fetch_data
// ---------------------------------------------------------------------------

TEST_F(MountDirectoryTest, FetchDataReturnsFileContents) {
    MountDirectory mount(temp_dir);
    mount.load();

    auto data = mount.fetch_data("hello.txt");
    std::string content(data.begin(), data.end());
    EXPECT_EQ(content, "Hello, world!");
}

TEST_F(MountDirectoryTest, FetchDataReturnsNestedFileContents) {
    MountDirectory mount(temp_dir);
    mount.load();

    auto data = mount.fetch_data("subdir/nested.txt");
    std::string content(data.begin(), data.end());
    EXPECT_EQ(content, "nested content");
}

TEST_F(MountDirectoryTest, FetchDataReturnsBinaryContents) {
    MountDirectory mount(temp_dir);
    mount.load();

    auto data = mount.fetch_data("binary.bin");
    ASSERT_EQ(data.size(), 4u);
    EXPECT_EQ(data[0], 0x00);
    EXPECT_EQ(data[1], 0x01);
    EXPECT_EQ(data[2], 0x02);
    EXPECT_EQ(data[3], 0x03);
}

TEST_F(MountDirectoryTest, FetchDataThrowsForNonexistentFile) {
    MountDirectory mount(temp_dir);
    mount.load();

    EXPECT_THROW(mount.fetch_data("no_such_file.txt"), std::runtime_error);
}

TEST_F(MountDirectoryTest, FetchDataReturnsEmptyForEmptyFile) {
    write_file(temp_dir / "empty.txt", "");

    MountDirectory mount(temp_dir);
    mount.load();

    auto data = mount.fetch_data("empty.txt");
    EXPECT_TRUE(data.empty());
}
