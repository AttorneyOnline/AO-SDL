#include <utils/StringHelpers.h>

#include <gtest/gtest.h>

TEST(StringHelpers, TrimSongName_EmptyInput) {
    EXPECT_EQ(StringHelpers::trim_song_name(""), "");
}

TEST(StringHelpers, TrimSongName_NoDirectoryNoExt) {
    EXPECT_EQ(StringHelpers::trim_song_name("Logic & Trick"), "Logic & Trick");
}

TEST(StringHelpers, TrimSongName_NoDirectory) {
    EXPECT_EQ(StringHelpers::trim_song_name("Logic & Trick.opus"), "Logic & Trick");
}

TEST(StringHelpers, TrimSongName_OneDirectoryNoExt) {
    EXPECT_EQ(StringHelpers::trim_song_name("Logic/Logic & Trick"), "Logic & Trick");
}

TEST(StringHelpers, TrimSongName_MultiDirectoryNoExt) {
    EXPECT_EQ(StringHelpers::trim_song_name("Ace Attorney/Logic/Logic & Trick"), "Logic & Trick");
}

TEST(StringHelpers, TrimSongName_OneDirectory) {
    EXPECT_EQ(StringHelpers::trim_song_name("Logic/Logic & Trick.opus"), "Logic & Trick");
}

TEST(StringHelpers, TrimSongName_MultiDirectory) {
    EXPECT_EQ(StringHelpers::trim_song_name("Ace Attorney/Logic/Logic & Trick.opus"), "Logic & Trick");
}
