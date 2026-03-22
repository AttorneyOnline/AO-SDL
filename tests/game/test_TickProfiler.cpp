#include "game/TickProfiler.h"

#include <gtest/gtest.h>
#include <chrono>
#include <thread>

// ---------------------------------------------------------------------------
// Basic construction
// ---------------------------------------------------------------------------

TEST(TickProfiler, EmptyProfilerHasNoEntries) {
    TickProfiler profiler;
    auto result = profiler.entries();
    EXPECT_TRUE(result.empty());
}

// ---------------------------------------------------------------------------
// Section registration
// ---------------------------------------------------------------------------

TEST(TickProfiler, AddSectionReturnsSequentialIndices) {
    TickProfiler profiler;
    int i0 = profiler.add_section("alpha");
    int i1 = profiler.add_section("beta");
    int i2 = profiler.add_section("gamma");
    EXPECT_EQ(i0, 0);
    EXPECT_EQ(i1, 1);
    EXPECT_EQ(i2, 2);
}

TEST(TickProfiler, EntriesReturnsAllRegisteredSections) {
    TickProfiler profiler;
    profiler.add_section("first");
    profiler.add_section("second");
    profiler.add_section("third");
    auto result = profiler.entries();
    ASSERT_EQ(result.size(), 3u);
}

TEST(TickProfiler, SectionNamesArePreservedInEntries) {
    TickProfiler profiler;
    profiler.add_section("events");
    profiler.add_section("emote");
    profiler.add_section("textbox");
    auto result = profiler.entries();
    ASSERT_EQ(result.size(), 3u);
    // names are const char* — compare as strings
    EXPECT_STREQ(result[0].name, "events");
    EXPECT_STREQ(result[1].name, "emote");
    EXPECT_STREQ(result[2].name, "textbox");
}

TEST(TickProfiler, EntriesPreserveInsertionOrder) {
    TickProfiler profiler;
    profiler.add_section("z_last");
    profiler.add_section("a_first");
    auto result = profiler.entries();
    ASSERT_EQ(result.size(), 2u);
    EXPECT_STREQ(result[0].name, "z_last");
    EXPECT_STREQ(result[1].name, "a_first");
}

// ---------------------------------------------------------------------------
// Duration tracking
// ---------------------------------------------------------------------------

TEST(TickProfiler, UnusedSectionHasZeroDuration) {
    TickProfiler profiler;
    profiler.add_section("idle");
    auto result = profiler.entries();
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].us->load(std::memory_order_relaxed), 0);
}

TEST(TickProfiler, ScopedSectionMeasuresNonZeroDuration) {
    TickProfiler profiler;
    int idx = profiler.add_section("work");

    {
        auto guard = profiler.scope(idx);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    auto result = profiler.entries();
    ASSERT_EQ(result.size(), 1u);
    int duration = result[0].us->load(std::memory_order_relaxed);
    // Sleeping 1ms should yield at least some microseconds
    EXPECT_GT(duration, 0);
}

TEST(TickProfiler, DurationRecordedAfterScopeGuardDestroyed) {
    TickProfiler profiler;
    int idx = profiler.add_section("section");

    {
        auto guard = profiler.scope(idx);
        // While the guard is alive, duration may still be 0 (not yet stored)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        // Guard destructor has not run yet
    }
    // Guard is now destroyed — duration should be recorded
    auto result = profiler.entries();
    ASSERT_EQ(result.size(), 1u);
    int duration = result[0].us->load(std::memory_order_relaxed);
    EXPECT_GT(duration, 0);
}

// ---------------------------------------------------------------------------
// Multiple sections are independent
// ---------------------------------------------------------------------------

TEST(TickProfiler, MultipleSectionsAreIndependent) {
    TickProfiler profiler;
    int fast = profiler.add_section("fast");
    int slow = profiler.add_section("slow");

    {
        auto guard = profiler.scope(fast);
        // Minimal work — should be very quick
    }
    {
        auto guard = profiler.scope(slow);
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    auto result = profiler.entries();
    ASSERT_EQ(result.size(), 2u);

    int fast_us = result[0].us->load(std::memory_order_relaxed);
    int slow_us = result[1].us->load(std::memory_order_relaxed);

    // The slow section should have a meaningfully larger duration
    EXPECT_GT(slow_us, fast_us);
    // The slow section should be at least 1ms (1000 us)
    EXPECT_GE(slow_us, 1000);
}

TEST(TickProfiler, OnlyScopedSectionIsAffected) {
    TickProfiler profiler;
    int a = profiler.add_section("a");
    profiler.add_section("b");

    {
        auto guard = profiler.scope(a);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    auto result = profiler.entries();
    ASSERT_EQ(result.size(), 2u);
    int a_us = result[0].us->load(std::memory_order_relaxed);
    int b_us = result[1].us->load(std::memory_order_relaxed);

    EXPECT_GT(a_us, 0);
    EXPECT_EQ(b_us, 0);
}

// ---------------------------------------------------------------------------
// Re-entry behavior: scope() overwrites (does not accumulate)
// ---------------------------------------------------------------------------

TEST(TickProfiler, ReenteringSectionOverwritesPreviousDuration) {
    TickProfiler profiler;
    int idx = profiler.add_section("reenter");

    // First measurement: sleep 5ms
    {
        auto guard = profiler.scope(idx);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    auto result = profiler.entries();
    int first_duration = result[0].us->load(std::memory_order_relaxed);
    EXPECT_GE(first_duration, 4000); // at least ~4ms

    // Second measurement: minimal work (should overwrite, not accumulate)
    {
        auto guard = profiler.scope(idx);
        // No sleep — near-zero work
    }
    int second_duration = result[0].us->load(std::memory_order_relaxed);

    // If it accumulated, second_duration would be >= first_duration.
    // Since it overwrites, second_duration should be much smaller.
    EXPECT_LT(second_duration, first_duration);
}

// ---------------------------------------------------------------------------
// ProfileEntry pointer stability
// ---------------------------------------------------------------------------

TEST(TickProfiler, EntryPointersAreStableAcrossCalls) {
    TickProfiler profiler;
    profiler.add_section("stable");

    auto entries1 = profiler.entries();
    auto entries2 = profiler.entries();

    ASSERT_EQ(entries1.size(), 1u);
    ASSERT_EQ(entries2.size(), 1u);

    // The atomic pointer should point to the same underlying storage
    EXPECT_EQ(entries1[0].us, entries2[0].us);
    EXPECT_EQ(entries1[0].name, entries2[0].name);
}

TEST(TickProfiler, EntryReflectsLiveDurationUpdates) {
    TickProfiler profiler;
    int idx = profiler.add_section("live");

    // Grab entries before any scope runs
    auto result = profiler.entries();
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].us->load(std::memory_order_relaxed), 0);

    // Run a scoped section
    {
        auto guard = profiler.scope(idx);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // The same pointer from earlier should now show the updated value,
    // since entries() returns pointers into the profiler's internal storage.
    int updated = result[0].us->load(std::memory_order_relaxed);
    EXPECT_GT(updated, 0);
}

// ---------------------------------------------------------------------------
// ScopedSection is move-only (not copyable)
// ---------------------------------------------------------------------------

TEST(TickProfiler, ScopedSectionIsNotCopyable) {
    EXPECT_FALSE(std::is_copy_constructible_v<TickProfiler::ScopedSection>);
    EXPECT_FALSE(std::is_copy_assignable_v<TickProfiler::ScopedSection>);
}

// ---------------------------------------------------------------------------
// Single section with multiple adds
// ---------------------------------------------------------------------------

TEST(TickProfiler, ManySectionsAllTrackedIndependently) {
    TickProfiler profiler;
    constexpr int N = 10;
    int indices[N];
    for (int i = 0; i < N; ++i) {
        // Use string literals that persist (static storage)
        static const char* names[] = {"s0", "s1", "s2", "s3", "s4",
                                       "s5", "s6", "s7", "s8", "s9"};
        indices[i] = profiler.add_section(names[i]);
        EXPECT_EQ(indices[i], i);
    }

    // Time only the middle section
    {
        auto guard = profiler.scope(indices[5]);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    auto result = profiler.entries();
    ASSERT_EQ(result.size(), static_cast<size_t>(N));

    for (int i = 0; i < N; ++i) {
        int us = result[i].us->load(std::memory_order_relaxed);
        if (i == 5) {
            EXPECT_GT(us, 0) << "Section 5 should have been timed";
        } else {
            EXPECT_EQ(us, 0) << "Section " << i << " should be untouched";
        }
    }
}
