#include "event/BackgroundEvent.h"

#include <gtest/gtest.h>
#include <string>

// ---------------------------------------------------------------------------
// Constructor and getters
// ---------------------------------------------------------------------------

TEST(BackgroundEvent, ConstructorStoresValues) {
    BackgroundEvent ev("gs4", "def");
    EXPECT_EQ(ev.get_background(), "gs4");
    EXPECT_EQ(ev.get_position(), "def");
}

TEST(BackgroundEvent, EmptyPosition) {
    BackgroundEvent ev("default", "");
    EXPECT_EQ(ev.get_background(), "default");
    EXPECT_EQ(ev.get_position(), "");
}

TEST(BackgroundEvent, EmptyBackgroundAndPosition) {
    BackgroundEvent ev("", "");
    EXPECT_EQ(ev.get_background(), "");
    EXPECT_EQ(ev.get_position(), "");
}

// ---------------------------------------------------------------------------
// to_string
// ---------------------------------------------------------------------------

TEST(BackgroundEvent, ToStringContainsFields) {
    BackgroundEvent ev("gs4", "pro");
    std::string s = ev.to_string();
    EXPECT_NE(s.find("gs4"), std::string::npos);
    EXPECT_NE(s.find("pro"), std::string::npos);
}

TEST(BackgroundEvent, ToStringWithEmptyPosition) {
    BackgroundEvent ev("default", "");
    std::string s = ev.to_string();
    EXPECT_NE(s.find("default"), std::string::npos);
}
