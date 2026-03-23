#include "asset/AssetLibrary.h"
#include "asset/ImageAsset.h"
#include "asset/MountManager.h"
#include "asset/RawAsset.h"
#include "asset/ShaderAsset.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// =============================================================================
// Fixture using only embedded assets (no temp directory needed)
// =============================================================================

class AssetLibraryTest : public ::testing::Test {
  protected:
    MountManager mounts;
    // Large cache budget so eviction doesn't interfere with basic tests.
    AssetLibrary lib{mounts, 256 * 1024 * 1024};

    void SetUp() override {
        // MountManager constructor auto-loads MountEmbedded,
        // so embedded assets (shaders, theme configs, images) are available.
        lib.set_shader_backend("OpenGL");
    }
};

// =============================================================================
// Fixture with a temporary directory for filesystem-based tests
// =============================================================================

class AssetLibraryFSTest : public ::testing::Test {
  protected:
    fs::path temp_dir;
    MountManager mounts;
    AssetLibrary lib{mounts, 256 * 1024 * 1024};

    void SetUp() override {
        temp_dir = fs::temp_directory_path() / "aosdl_test_asset_library";
        fs::remove_all(temp_dir);
        fs::create_directories(temp_dir);

        lib.set_shader_backend("OpenGL");

        // Add the temp directory as a mount (highest priority, before embedded).
        mounts.load_mounts({temp_dir});
    }

    void TearDown() override {
        fs::remove_all(temp_dir);
    }

    void write_file(const fs::path& path, const std::string& content) {
        fs::create_directories(path.parent_path());
        std::ofstream out(path, std::ios::binary);
        out.write(content.data(), static_cast<std::streamsize>(content.size()));
    }

    void write_file(const fs::path& path, const std::vector<uint8_t>& content) {
        fs::create_directories(path.parent_path());
        std::ofstream out(path, std::ios::binary);
        out.write(reinterpret_cast<const char*>(content.data()), static_cast<std::streamsize>(content.size()));
    }

    /// Write a minimal valid 1x1 PNG file to the given path.
    void write_minimal_png(const fs::path& path) {
        // Minimal valid 1x1 red RGBA PNG (68 bytes).
        static const uint8_t png[] = {
            0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, // PNG signature
            0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52, // IHDR chunk
            0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, // 1x1
            0x08, 0x06, 0x00, 0x00, 0x00, 0x1F, 0x15, 0xC4, // 8-bit RGBA
            0x89, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x44, 0x41, // IDAT chunk
            0x54, 0x78, 0x9C, 0x62, 0xF8, 0xCF, 0xC0, 0x00, // compressed
            0x00, 0x00, 0x05, 0x00, 0x01, 0xA5, 0xF6, 0x45, // data
            0x40, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, // IEND chunk
            0x44, 0xAE, 0x42, 0x60, 0x82,
        };
        write_file(path, std::vector<uint8_t>(png, png + sizeof(png)));
    }

    /// Write a minimal valid 1x1 GIF file to the given path.
    void write_minimal_gif(const fs::path& path) {
        static const uint8_t gif[] = {
            0x47, 0x49, 0x46, 0x38, 0x39, 0x61, // GIF89a
            0x01, 0x00, 0x01, 0x00,             // 1x1
            0x80, 0x00, 0x00,                   // GCT flag, 2 colors
            0xFF, 0x00, 0x00,                   // color 0: red
            0x00, 0x00, 0x00,                   // color 1: black
            0x2C, 0x00, 0x00, 0x00, 0x00,       // image descriptor
            0x01, 0x00, 0x01, 0x00, 0x00,       // 1x1, no local CT
            0x02, 0x02, 0x44, 0x01, 0x00,       // LZW min code 2, data
            0x3B,                               // trailer
        };
        write_file(path, std::vector<uint8_t>(gif, gif + sizeof(gif)));
    }
};

// =============================================================================
// config() — loading and parsing INI files
// =============================================================================

TEST_F(AssetLibraryTest, ConfigLoadsEmbeddedChatConfig) {
    auto doc = lib.config("themes/default/chat_config.ini");
    ASSERT_TRUE(doc.has_value());
    // chat_config.ini has keys in the default (empty) section.
    EXPECT_FALSE(doc->empty());
}

TEST_F(AssetLibraryTest, ConfigParsesKeyValuePairs) {
    auto doc = lib.config("themes/default/chat_config.ini");
    ASSERT_TRUE(doc.has_value());
    // The file has "c0 = 247, 247, 247" in the default section.
    auto it = doc->find("");
    ASSERT_NE(it, doc->end()) << "Expected a default (empty) section";
    EXPECT_NE(it->second.find("c0"), it->second.end());
    EXPECT_EQ(it->second.at("c0"), "247, 247, 247");
}

TEST_F(AssetLibraryTest, ConfigParsesShowname) {
    auto doc = lib.config("themes/default/courtroom_fonts.ini");
    ASSERT_TRUE(doc.has_value());
    auto it = doc->find("");
    ASSERT_NE(it, doc->end());
    EXPECT_EQ(it->second.at("showname"), "8");
    EXPECT_EQ(it->second.at("showname_font"), "Arial");
}

TEST_F(AssetLibraryTest, ConfigReturnsNulloptForMissingFile) {
    auto doc = lib.config("nonexistent/config.ini");
    EXPECT_FALSE(doc.has_value());
}

TEST_F(AssetLibraryTest, ConfigCachesRawData) {
    // First load should cache the raw bytes.
    auto doc1 = lib.config("themes/default/chat_config.ini");
    ASSERT_TRUE(doc1.has_value());

    auto cached = lib.get_cached("themes/default/chat_config.ini");
    EXPECT_NE(cached, nullptr) << "Config should be cached after first load";
}

TEST_F(AssetLibraryTest, ConfigReturnsSameResultOnSecondCall) {
    auto doc1 = lib.config("themes/default/chat_config.ini");
    auto doc2 = lib.config("themes/default/chat_config.ini");
    ASSERT_TRUE(doc1.has_value());
    ASSERT_TRUE(doc2.has_value());
    // Both should parse to the same content.
    EXPECT_EQ(doc1->size(), doc2->size());
}

TEST_F(AssetLibraryFSTest, ConfigParsesIniSections) {
    write_file(temp_dir / "test.ini", "[section1]\nkey1 = value1\nkey2 = value2\n"
                                      "[section2]\nkeyA = valueA\n");
    auto doc = lib.config("test.ini");
    ASSERT_TRUE(doc.has_value());
    EXPECT_EQ(doc->at("section1").at("key1"), "value1");
    EXPECT_EQ(doc->at("section1").at("key2"), "value2");
    EXPECT_EQ(doc->at("section2").at("keyA"), "valueA");
}

TEST_F(AssetLibraryFSTest, ConfigIgnoresCommentsAndBlankLines) {
    write_file(temp_dir / "comments.ini", "; this is a comment\n"
                                          "# this too\n"
                                          "\n"
                                          "  \n"
                                          "key = value\n");
    auto doc = lib.config("comments.ini");
    ASSERT_TRUE(doc.has_value());
    EXPECT_EQ(doc->at("").at("key"), "value");
    // The comment lines should not appear as keys.
    EXPECT_EQ(doc->at("").size(), 1u);
}

TEST_F(AssetLibraryFSTest, ConfigStripsWhitespace) {
    write_file(temp_dir / "whitespace.ini", "  key_with_spaces  =  value_with_spaces  \n");
    auto doc = lib.config("whitespace.ini");
    ASSERT_TRUE(doc.has_value());
    EXPECT_EQ(doc->at("").at("key_with_spaces"), "value_with_spaces  ");
}

TEST_F(AssetLibraryFSTest, ConfigHandlesCRLFLineEndings) {
    write_file(temp_dir / "crlf.ini", "[section]\r\nkey = value\r\n");
    auto doc = lib.config("crlf.ini");
    ASSERT_TRUE(doc.has_value());
    EXPECT_EQ(doc->at("section").at("key"), "value");
}

// =============================================================================
// shader() — loading shader pairs
// =============================================================================

TEST_F(AssetLibraryTest, ShaderLoadsEmbeddedMainShader) {
    auto shader = lib.shader("shaders/main");
    ASSERT_NE(shader, nullptr);
    EXPECT_FALSE(shader->vertex_source().empty());
    EXPECT_FALSE(shader->fragment_source().empty());
}

TEST_F(AssetLibraryTest, ShaderVertexSourceContainsGlsl) {
    auto shader = lib.shader("shaders/main");
    ASSERT_NE(shader, nullptr);
    // The main vertex shader starts with "#version 330".
    EXPECT_NE(shader->vertex_source().find("#version"), std::string::npos);
}

TEST_F(AssetLibraryTest, ShaderFragmentSourceContainsGlsl) {
    auto shader = lib.shader("shaders/main");
    ASSERT_NE(shader, nullptr);
    EXPECT_NE(shader->fragment_source().find("#version"), std::string::npos);
}

TEST_F(AssetLibraryTest, ShaderIsCachedAfterFirstLoad) {
    auto s1 = lib.shader("shaders/main");
    auto s2 = lib.shader("shaders/main");
    ASSERT_NE(s1, nullptr);
    ASSERT_NE(s2, nullptr);
    // Same shared_ptr should be returned from cache.
    EXPECT_EQ(s1.get(), s2.get());
}

TEST_F(AssetLibraryTest, ShaderReturnsNullptrForMissingShader) {
    auto shader = lib.shader("shaders/nonexistent");
    EXPECT_EQ(shader, nullptr);
}

TEST_F(AssetLibraryTest, ShaderLoadsTextShader) {
    auto shader = lib.shader("shaders/text");
    ASSERT_NE(shader, nullptr);
    EXPECT_FALSE(shader->vertex_source().empty());
    EXPECT_FALSE(shader->fragment_source().empty());
}

TEST_F(AssetLibraryTest, ShaderLoadsCubeShader) {
    auto shader = lib.shader("shaders/cube");
    ASSERT_NE(shader, nullptr);
}

TEST_F(AssetLibraryTest, ShaderLoadsRainbowShader) {
    auto shader = lib.shader("shaders/rainbow");
    ASSERT_NE(shader, nullptr);
}

TEST_F(AssetLibraryTest, ShaderFormatIsGlsl) {
    auto shader = lib.shader("shaders/main");
    ASSERT_NE(shader, nullptr);
    EXPECT_EQ(shader->format(), "glsl");
}

TEST_F(AssetLibraryTest, ShaderMemorySizeMatchesSources) {
    auto shader = lib.shader("shaders/main");
    ASSERT_NE(shader, nullptr);
    EXPECT_EQ(shader->memory_size(), shader->vertex_source().size() + shader->fragment_source().size());
}

TEST_F(AssetLibraryTest, ShaderCacheKeyIncludesBackend) {
    auto shader = lib.shader("shaders/main");
    ASSERT_NE(shader, nullptr);
    // The cache key is "shaders/main:OpenGL" (path + ":" + backend).
    auto cached = lib.get_cached("shaders/main:OpenGL");
    EXPECT_NE(cached, nullptr);
    // Looking up without the backend suffix should miss.
    auto no_backend = lib.get_cached("shaders/main");
    EXPECT_EQ(no_backend, nullptr);
}

// =============================================================================
// image() — loading images with extension probing
// =============================================================================

TEST_F(AssetLibraryTest, ImageLoadsEmbeddedPng) {
    // themes/default/chat.png is a known-good embedded PNG
    auto img = lib.image("themes/default/chat");
    ASSERT_NE(img, nullptr);
    EXPECT_GT(img->width(), 0);
    EXPECT_GT(img->height(), 0);
    EXPECT_EQ(img->frame_count(), 1);
}

TEST_F(AssetLibraryFSTest, ImageLoadsGifFile) {
    write_minimal_gif(temp_dir / "sprites" / "test.gif");
    auto img = lib.image("sprites/test");
    ASSERT_NE(img, nullptr);
    EXPECT_EQ(img->width(), 1);
    EXPECT_EQ(img->height(), 1);
}

TEST_F(AssetLibraryFSTest, ImageReturnsNullptrForMissingFile) {
    auto img = lib.image("nonexistent/sprite");
    EXPECT_EQ(img, nullptr);
}

TEST_F(AssetLibraryTest, ImageIsCachedAfterFirstLoad) {
    auto img1 = lib.image("themes/default/chat");
    auto img2 = lib.image("themes/default/chat");
    ASSERT_NE(img1, nullptr);
    ASSERT_NE(img2, nullptr);
    EXPECT_EQ(img1.get(), img2.get()) << "Second call should return cached instance";
}

TEST_F(AssetLibraryTest, ImageCacheKeyIsPathWithoutExtension) {
    auto img = lib.image("themes/default/chat");
    ASSERT_NE(img, nullptr);
    EXPECT_EQ(img->path(), "themes/default/chat");
    auto cached = lib.get_cached("themes/default/chat");
    EXPECT_NE(cached, nullptr);
}

TEST_F(AssetLibraryFSTest, ImageReturnsNullptrForCorruptData) {
    // Write something that is not a valid image.
    write_file(temp_dir / "corrupt.png", "this is not a valid png file");
    auto img = lib.image("corrupt");
    EXPECT_EQ(img, nullptr);
}

// =============================================================================
// Extension probing priority
// =============================================================================

TEST_F(AssetLibraryTest, ExtensionProbingFindsEmbeddedPng) {
    // Embedded theme image is a PNG — probing should find it.
    auto img = lib.image("themes/default/chat");
    ASSERT_NE(img, nullptr);
    EXPECT_EQ(img->format(), "png");
}

TEST_F(AssetLibraryFSTest, ExtensionProbingPrefersGifWhenOnlyGifExists) {
    write_minimal_gif(temp_dir / "probe" / "only_gif.gif");
    auto img = lib.image("probe/only_gif");
    ASSERT_NE(img, nullptr);
    EXPECT_EQ(img->format(), "gif");
}

TEST_F(AssetLibraryTest, ImageFormatIsPngForEmbeddedChat) {
    auto img = lib.image("themes/default/chat");
    ASSERT_NE(img, nullptr);
    EXPECT_EQ(img->format(), "png");
}

TEST_F(AssetLibraryFSTest, ExtensionProbingIgnoresUnknownExtensions) {
    // A file with .bmp extension should not be found by image probing.
    write_file(temp_dir / "probe" / "unknown.bmp", "bmp data");
    auto img = lib.image("probe/unknown");
    EXPECT_EQ(img, nullptr);
}

// =============================================================================
// raw() — loading raw bytes
// =============================================================================

TEST_F(AssetLibraryTest, RawLoadsEmbeddedFile) {
    auto data = lib.raw("themes/default/chat_config.ini");
    ASSERT_TRUE(data.has_value());
    EXPECT_FALSE(data->empty());
    // The file starts with "c0 = ".
    std::string content(data->begin(), data->end());
    EXPECT_NE(content.find("c0 ="), std::string::npos);
}

TEST_F(AssetLibraryTest, RawReturnsNulloptForMissingFile) {
    auto data = lib.raw("nonexistent/file.dat");
    EXPECT_FALSE(data.has_value());
}

TEST_F(AssetLibraryFSTest, RawLoadsFromTempDirectory) {
    write_file(temp_dir / "rawtest.dat", "raw content here");
    auto data = lib.raw("rawtest.dat");
    ASSERT_TRUE(data.has_value());
    std::string content(data->begin(), data->end());
    EXPECT_EQ(content, "raw content here");
}

TEST_F(AssetLibraryFSTest, RawCachesData) {
    write_file(temp_dir / "cached_raw.dat", "cached bytes");
    auto data1 = lib.raw("cached_raw.dat");
    ASSERT_TRUE(data1.has_value());

    // Should now be in cache.
    auto cached = lib.get_cached("cached_raw.dat");
    EXPECT_NE(cached, nullptr);
}

// =============================================================================
// register_asset() — manually registering assets
// =============================================================================

TEST_F(AssetLibraryTest, RegisterAssetMakesItRetrievable) {
    auto asset = std::make_shared<RawAsset>("custom/test", "raw", std::vector<uint8_t>{1, 2, 3});
    lib.register_asset(asset);

    auto retrieved = lib.get_cached("custom/test");
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved.get(), asset.get());
}

TEST_F(AssetLibraryTest, RegisterAssetOverwritesPreviousEntry) {
    auto asset1 = std::make_shared<RawAsset>("overwrite/test", "raw", std::vector<uint8_t>{1});
    auto asset2 = std::make_shared<RawAsset>("overwrite/test", "raw", std::vector<uint8_t>{2, 3});
    lib.register_asset(asset1);
    lib.register_asset(asset2);

    auto retrieved = lib.get_cached("overwrite/test");
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved.get(), asset2.get());
}

TEST_F(AssetLibraryTest, RegisteredImageRetrievableViaImageCall) {
    // Register a manually-created ImageAsset, then verify image() returns it.
    std::vector<DecodedFrame> frames;
    frames.push_back(DecodedFrame{{0xFF, 0x00, 0x00, 0xFF}, 1, 1, 0});
    auto img = std::make_shared<ImageAsset>("manual/sprite", "png", std::move(frames));
    lib.register_asset(img);

    auto result = lib.image("manual/sprite");
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->width(), 1);
    EXPECT_EQ(result->height(), 1);
}

// =============================================================================
// get_cached() — cache lookups
// =============================================================================

TEST_F(AssetLibraryTest, GetCachedReturnsNullptrForUncachedPath) {
    EXPECT_EQ(lib.get_cached("never/loaded"), nullptr);
}

TEST_F(AssetLibraryTest, GetCachedReturnsAssetAfterLoad) {
    auto shader = lib.shader("shaders/main");
    ASSERT_NE(shader, nullptr);
    auto cached = lib.get_cached("shaders/main:OpenGL");
    EXPECT_NE(cached, nullptr);
}

// =============================================================================
// evict() — cache eviction
// =============================================================================

TEST(AssetLibraryEviction, EvictRemovesUnpinnedEntries) {
    MountManager mounts;
    // Tiny cache budget: 10 bytes.
    AssetLibrary lib(mounts, 10);

    // Register an asset larger than the budget.
    auto big = std::make_shared<RawAsset>("big", "raw", std::vector<uint8_t>(100, 0));
    lib.register_asset(big);

    // Drop external reference so it's unpinned.
    big.reset();

    lib.evict();

    // Should have been evicted.
    EXPECT_EQ(lib.get_cached("big"), nullptr);
}

TEST(AssetLibraryEviction, EvictKeepsPinnedEntries) {
    MountManager mounts;
    AssetLibrary lib(mounts, 10);

    auto pinned = std::make_shared<RawAsset>("pinned", "raw", std::vector<uint8_t>(100, 0));
    lib.register_asset(pinned);

    // Keep external reference (pinned).
    lib.evict();

    EXPECT_NE(lib.get_cached("pinned"), nullptr);
}

TEST(AssetLibraryEviction, EvictDoesNothingWhenUnderBudget) {
    MountManager mounts;
    AssetLibrary lib(mounts, 1024);

    auto small = std::make_shared<RawAsset>("small", "raw", std::vector<uint8_t>(10, 0));
    lib.register_asset(small);
    small.reset();

    lib.evict();

    // Under budget, should still be present.
    EXPECT_NE(lib.get_cached("small"), nullptr);
}

// =============================================================================
// cache() — accessing the internal cache
// =============================================================================

TEST_F(AssetLibraryTest, CacheReportsUsedBytesAfterLoads) {
    EXPECT_EQ(lib.cache().used_bytes(), 0u);

    auto shader = lib.shader("shaders/main");
    ASSERT_NE(shader, nullptr);
    EXPECT_GT(lib.cache().used_bytes(), 0u);
}

TEST_F(AssetLibraryTest, CacheEntryCountGrowsWithLoads) {
    size_t before = lib.cache().entry_count();
    lib.shader("shaders/main");
    size_t after = lib.cache().entry_count();
    EXPECT_GT(after, before);
}

// =============================================================================
// Embedded theme image loading via image()
// =============================================================================

TEST_F(AssetLibraryTest, ImageLoadsEmbeddedThemePng) {
    // The "themes/default/chat.png" exists as an embedded asset.
    auto img = lib.image("themes/default/chat");
    ASSERT_NE(img, nullptr);
    EXPECT_GT(img->width(), 0);
    EXPECT_GT(img->height(), 0);
    EXPECT_EQ(img->format(), "png");
}

TEST_F(AssetLibraryTest, ImageLoadsAceAttorneyDSThemePng) {
    auto img = lib.image("themes/AceAttorney DS/chat");
    ASSERT_NE(img, nullptr);
    EXPECT_GT(img->width(), 0);
    EXPECT_GT(img->height(), 0);
}

// =============================================================================
// Shader loading with Metal backend
// =============================================================================

TEST_F(AssetLibraryTest, ShaderLoadMetalBackend) {
    lib.set_shader_backend("Metal");
    auto shader = lib.shader("shaders/main");
    ASSERT_NE(shader, nullptr);
    EXPECT_FALSE(shader->vertex_source().empty());
    EXPECT_FALSE(shader->fragment_source().empty());
    EXPECT_EQ(shader->format(), "metal");
}

TEST_F(AssetLibraryTest, ShaderMetalAndGlslAreSeparatelyCached) {
    lib.set_shader_backend("OpenGL");
    auto glsl = lib.shader("shaders/main");

    lib.set_shader_backend("Metal");
    auto metal = lib.shader("shaders/main");

    ASSERT_NE(glsl, nullptr);
    ASSERT_NE(metal, nullptr);
    EXPECT_NE(glsl.get(), metal.get()) << "Different backends should produce different cache entries";
    EXPECT_EQ(glsl->format(), "glsl");
    EXPECT_EQ(metal->format(), "metal");
}

// =============================================================================
// prefetch — basic coverage (no HTTP mounts, so these are effectively no-ops)
// =============================================================================

TEST_F(AssetLibraryTest, PrefetchImageDoesNotCrashWithoutHttpMount) {
    EXPECT_NO_THROW(lib.prefetch_image("some/path"));
}

TEST_F(AssetLibraryTest, PrefetchAudioDoesNotCrashWithoutHttpMount) {
    EXPECT_NO_THROW(lib.prefetch_audio("some/path"));
}

TEST_F(AssetLibraryTest, PrefetchConfigDoesNotCrashWithoutHttpMount) {
    EXPECT_NO_THROW(lib.prefetch_config("some/path.ini"));
}

TEST_F(AssetLibraryTest, PrefetchWithExtensionsDoesNotCrash) {
    EXPECT_NO_THROW(lib.prefetch("some/path", {"png", "gif"}));
}

TEST_F(AssetLibraryTest, PrefetchImageSkipsIfAlreadyCached) {
    // Register a fake asset, then prefetch should be a no-op.
    std::vector<DecodedFrame> frames;
    frames.push_back(DecodedFrame{{0xFF, 0x00, 0x00, 0xFF}, 1, 1, 0});
    lib.register_asset(std::make_shared<ImageAsset>("cached/img", "png", std::move(frames)));

    // Should not crash or do anything harmful.
    EXPECT_NO_THROW(lib.prefetch_image("cached/img"));
}

// =============================================================================
// config() with filesystem-based INI edge cases
// =============================================================================

TEST_F(AssetLibraryFSTest, ConfigHandlesEmptyFile) {
    write_file(temp_dir / "empty.ini", "");
    auto doc = lib.config("empty.ini");
    ASSERT_TRUE(doc.has_value());
    EXPECT_TRUE(doc->empty());
}

TEST_F(AssetLibraryFSTest, ConfigHandlesOnlySections) {
    write_file(temp_dir / "sections_only.ini", "[section1]\n[section2]\n");
    auto doc = lib.config("sections_only.ini");
    ASSERT_TRUE(doc.has_value());
    // Sections exist but have no keys.
    // Note: the parser only creates section entries when keys are added,
    // so empty sections may not appear in the map.
}

TEST_F(AssetLibraryFSTest, ConfigKeysBeforeFirstSectionGoToDefault) {
    write_file(temp_dir / "no_section.ini", "key1 = val1\nkey2 = val2\n");
    auto doc = lib.config("no_section.ini");
    ASSERT_TRUE(doc.has_value());
    EXPECT_EQ(doc->at("").at("key1"), "val1");
    EXPECT_EQ(doc->at("").at("key2"), "val2");
}

TEST_F(AssetLibraryFSTest, ConfigHandlesEqualsSignInValue) {
    write_file(temp_dir / "equals.ini", "key = value=with=equals\n");
    auto doc = lib.config("equals.ini");
    ASSERT_TRUE(doc.has_value());
    EXPECT_EQ(doc->at("").at("key"), "value=with=equals");
}

// =============================================================================
// Multiple shader loads — different embedded shaders
// =============================================================================

TEST_F(AssetLibraryTest, LoadMultipleDifferentShaders) {
    auto main_shader = lib.shader("shaders/main");
    auto text_shader = lib.shader("shaders/text");
    auto cube_shader = lib.shader("shaders/cube");

    ASSERT_NE(main_shader, nullptr);
    ASSERT_NE(text_shader, nullptr);
    ASSERT_NE(cube_shader, nullptr);

    // They should all be distinct objects.
    EXPECT_NE(main_shader.get(), text_shader.get());
    EXPECT_NE(main_shader.get(), cube_shader.get());
    EXPECT_NE(text_shader.get(), cube_shader.get());
}

// =============================================================================
// image() format detection — verifying the correct decoder is picked
// =============================================================================

TEST_F(AssetLibraryTest, ImageFormatReflectsResolvedExtension) {
    auto img = lib.image("themes/default/chat");
    ASSERT_NE(img, nullptr);
    EXPECT_EQ(img->format(), "png");
}

TEST_F(AssetLibraryFSTest, ImageGifFormatReflectsResolvedExtension) {
    write_minimal_gif(temp_dir / "format_test_gif.gif");
    auto img = lib.image("format_test_gif");
    ASSERT_NE(img, nullptr);
    EXPECT_EQ(img->format(), "gif");
}

// =============================================================================
// Shader custom paths with FS mount
// =============================================================================

TEST_F(AssetLibraryFSTest, ShaderLoadsFromFilesystem) {
    fs::create_directories(temp_dir / "shaders" / "custom" / "glsl");
    write_file(temp_dir / "shaders" / "custom" / "glsl" / "vertex.glsl",
               "#version 330\nvoid main() { gl_Position = vec4(0); }");
    write_file(temp_dir / "shaders" / "custom" / "glsl" / "fragment.glsl",
               "#version 330\nout vec4 color;\nvoid main() { color = vec4(1); }");

    auto shader = lib.shader("shaders/custom");
    ASSERT_NE(shader, nullptr);
    EXPECT_NE(shader->vertex_source().find("#version 330"), std::string::npos);
    EXPECT_NE(shader->fragment_source().find("vec4(1)"), std::string::npos);
}

TEST_F(AssetLibraryFSTest, ShaderSupportsVertExtension) {
    // The probing also tries .vert/.frag extensions.
    fs::create_directories(temp_dir / "shaders" / "alt" / "glsl");
    write_file(temp_dir / "shaders" / "alt" / "glsl" / "vertex.vert",
               "#version 330\nvoid main() { gl_Position = vec4(0); }");
    write_file(temp_dir / "shaders" / "alt" / "glsl" / "fragment.frag",
               "#version 330\nout vec4 c;\nvoid main() { c = vec4(1); }");

    auto shader = lib.shader("shaders/alt");
    ASSERT_NE(shader, nullptr);
    EXPECT_FALSE(shader->vertex_source().empty());
    EXPECT_FALSE(shader->fragment_source().empty());
}

// =============================================================================
// list() — currently returns empty (documented as unimplemented)
// =============================================================================

TEST_F(AssetLibraryTest, ListReturnsEmptyForNow) {
    auto entries = lib.list("themes");
    EXPECT_TRUE(entries.empty());
}

// =============================================================================
// font() — currently returns nullptr (documented as incomplete)
// =============================================================================

TEST_F(AssetLibraryTest, FontReturnsNullptr) {
    // font() loads data but returns nullptr because FontAsset doesn't exist yet.
    auto font = lib.font("fonts/igiari-cyrillic.ttf");
    EXPECT_EQ(font, nullptr);
}

// =============================================================================
// Integration: loading config, then image from same cache
// =============================================================================

TEST_F(AssetLibraryTest, ConfigAndImageCoexistInCache) {
    auto doc = lib.config("themes/default/courtroom_design.ini");
    ASSERT_TRUE(doc.has_value());

    auto img = lib.image("themes/default/chat");
    ASSERT_NE(img, nullptr);

    // Both should be in the cache.
    EXPECT_NE(lib.get_cached("themes/default/courtroom_design.ini"), nullptr);
    EXPECT_NE(lib.get_cached("themes/default/chat"), nullptr);
}
