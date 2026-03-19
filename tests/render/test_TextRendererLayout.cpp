#include "render/TextRenderer.h"

#include <gtest/gtest.h>
#include <cstdint>

// ---------------------------------------------------------------------------
// Construction and no-font state
// ---------------------------------------------------------------------------

TEST(TextRendererLayout, DefaultConstructedLineHeightIsZero) {
    TextRenderer tr;
    EXPECT_EQ(tr.line_height(), 0);
}

// ---------------------------------------------------------------------------
// load_font with nonexistent path
// ---------------------------------------------------------------------------

TEST(TextRendererLayout, LoadFontNonexistentPathReturnsFalse) {
    TextRenderer tr;
    EXPECT_FALSE(tr.load_font("/no/such/font/file.ttf", 16));
}

// ---------------------------------------------------------------------------
// load_font_memory with nullptr
// ---------------------------------------------------------------------------

TEST(TextRendererLayout, LoadFontMemoryNullptrReturnsFalse) {
    TextRenderer tr;
    EXPECT_FALSE(tr.load_font_memory(nullptr, 0, 16));
}

// ---------------------------------------------------------------------------
// set_sharp should not crash even without a loaded font
// ---------------------------------------------------------------------------

TEST(TextRendererLayout, SetSharpDoesNotCrash) {
    TextRenderer tr;
    EXPECT_NO_FATAL_FAILURE(tr.set_sharp(true));
    EXPECT_NO_FATAL_FAILURE(tr.set_sharp(false));
}
