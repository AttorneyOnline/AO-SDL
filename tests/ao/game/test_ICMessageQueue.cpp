#include "ao/game/ICMessageQueue.h"

#include <gtest/gtest.h>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace {

static ICMessage make_msg(const std::string& character = "Phoenix", const std::string& message = "Hello") {
    ICMessage msg;
    msg.character = character;
    msg.message = message;
    msg.emote = "normal";
    msg.pre_emote = "";
    msg.showname = character;
    msg.side = "def";
    msg.emote_mod = EmoteMod::IDLE;
    msg.desk_mod = DeskMod::CHAT;
    msg.flip = false;
    msg.char_id = 0;
    msg.text_color = 0;
    msg.screenshake = false;
    msg.realization = false;
    msg.additive = false;
    msg.objection_mod = 0;
    msg.sfx_name = "";
    msg.sfx_delay = 0;
    msg.sfx_looping = false;
    msg.frame_sfx = "";
    msg.immediate = false;
    return msg;
}

static ICMessage make_objection(const std::string& character = "Phoenix", int objection_mod = 1) {
    ICMessage msg = make_msg(character, "Objection!");
    msg.objection_mod = objection_mod;
    return msg;
}

class ICMessageQueueTest : public ::testing::Test {
  protected:
    ICMessageQueue queue;
};

} // namespace

// ---------------------------------------------------------------------------
// Empty queue
// ---------------------------------------------------------------------------

TEST_F(ICMessageQueueTest, EmptyQueueHasZeroPending) {
    EXPECT_EQ(queue.pending(), 0u);
}

TEST_F(ICMessageQueueTest, EmptyQueueIsNotPlaying) {
    EXPECT_FALSE(queue.is_playing());
}

TEST_F(ICMessageQueueTest, EmptyQueueNextReturnsNullopt) {
    EXPECT_EQ(queue.next(), std::nullopt);
}

// ---------------------------------------------------------------------------
// Enqueue single message
// ---------------------------------------------------------------------------

TEST_F(ICMessageQueueTest, EnqueueSingleMessageSetsPendingToOne) {
    queue.enqueue(make_msg());
    EXPECT_EQ(queue.pending(), 1u);
}

TEST_F(ICMessageQueueTest, EnqueueWhenNotPlayingMakesMessageReady) {
    queue.enqueue(make_msg());
    auto msg = queue.next();
    ASSERT_TRUE(msg.has_value());
    EXPECT_EQ(msg->character, "Phoenix");
    EXPECT_EQ(msg->message, "Hello");
}

// ---------------------------------------------------------------------------
// next() returns message and marks playing
// ---------------------------------------------------------------------------

TEST_F(ICMessageQueueTest, NextMarksPlaying) {
    queue.enqueue(make_msg());
    queue.next();
    EXPECT_TRUE(queue.is_playing());
}

TEST_F(ICMessageQueueTest, NextRemovesFromPending) {
    queue.enqueue(make_msg());
    queue.next();
    EXPECT_EQ(queue.pending(), 0u);
}

TEST_F(ICMessageQueueTest, NextReturnsNulloptWhenNothingReady) {
    queue.enqueue(make_msg());
    queue.next(); // consume the only message
    // No more messages ready
    EXPECT_EQ(queue.next(), std::nullopt);
}

// ---------------------------------------------------------------------------
// tick() with current_message_done
// ---------------------------------------------------------------------------

TEST_F(ICMessageQueueTest, TickWithDoneTrueReturnsTrueWhenQueuedMessageReady) {
    queue.enqueue(make_msg("Phoenix", "First"));
    queue.next(); // start playing first message

    queue.enqueue(make_msg("Edgeworth", "Second"));
    EXPECT_EQ(queue.pending(), 1u);

    // Tick with done=true, accumulate enough linger time (300ms)
    bool result = queue.tick(300, true);
    EXPECT_TRUE(result);

    // Now next() should give us the second message
    auto msg = queue.next();
    ASSERT_TRUE(msg.has_value());
    EXPECT_EQ(msg->character, "Edgeworth");
}

TEST_F(ICMessageQueueTest, TickWithDoneFalseReturnsFalse) {
    queue.enqueue(make_msg("Phoenix", "First"));
    queue.next();

    queue.enqueue(make_msg("Edgeworth", "Second"));

    // Message not done yet
    bool result = queue.tick(16, false);
    EXPECT_FALSE(result);
    EXPECT_TRUE(queue.is_playing());
}

TEST_F(ICMessageQueueTest, TickOnEmptyQueueWithNothingPlayingReturnsFalse) {
    // Nothing playing, nothing queued
    bool result = queue.tick(16, false);
    EXPECT_FALSE(result);
}

TEST_F(ICMessageQueueTest, TickWhenNotPlayingReturnsReadyState) {
    // Enqueue without playing - ready_ is true
    queue.enqueue(make_msg());
    // Not playing, so tick returns ready_ which is true
    bool result = queue.tick(16, false);
    EXPECT_TRUE(result);
}

// ---------------------------------------------------------------------------
// Multiple enqueue preserves FIFO order
// ---------------------------------------------------------------------------

TEST_F(ICMessageQueueTest, FIFOOrderPreserved) {
    queue.enqueue(make_msg("Phoenix", "First"));
    queue.enqueue(make_msg("Edgeworth", "Second"));
    queue.enqueue(make_msg("Maya", "Third"));
    EXPECT_EQ(queue.pending(), 3u);

    auto msg1 = queue.next();
    ASSERT_TRUE(msg1.has_value());
    EXPECT_EQ(msg1->character, "Phoenix");
    EXPECT_EQ(msg1->message, "First");

    // Complete first message with enough linger
    queue.tick(300, true);

    auto msg2 = queue.next();
    ASSERT_TRUE(msg2.has_value());
    EXPECT_EQ(msg2->character, "Edgeworth");
    EXPECT_EQ(msg2->message, "Second");

    // Complete second message with enough linger
    queue.tick(300, true);

    auto msg3 = queue.next();
    ASSERT_TRUE(msg3.has_value());
    EXPECT_EQ(msg3->character, "Maya");
    EXPECT_EQ(msg3->message, "Third");
}

TEST_F(ICMessageQueueTest, EnqueueWhilePlayingDoesNotMakeImmediatelyReady) {
    queue.enqueue(make_msg("Phoenix", "First"));
    queue.next(); // start playing

    queue.enqueue(make_msg("Edgeworth", "Second"));
    // Second message should be pending, not immediately available
    EXPECT_EQ(queue.pending(), 1u);
    // next() should return nullopt because ready_ is false while playing
    EXPECT_EQ(queue.next(), std::nullopt);
}

// ---------------------------------------------------------------------------
// clear() resets everything
// ---------------------------------------------------------------------------

TEST_F(ICMessageQueueTest, ClearResetsAllState) {
    queue.enqueue(make_msg("Phoenix", "First"));
    queue.next(); // mark as playing
    queue.enqueue(make_msg("Edgeworth", "Second"));

    EXPECT_TRUE(queue.is_playing());
    EXPECT_EQ(queue.pending(), 1u);

    queue.clear();

    EXPECT_FALSE(queue.is_playing());
    EXPECT_EQ(queue.pending(), 0u);
    EXPECT_EQ(queue.next(), std::nullopt);
}

TEST_F(ICMessageQueueTest, ClearOnEmptyQueueIsNoOp) {
    queue.clear();
    EXPECT_FALSE(queue.is_playing());
    EXPECT_EQ(queue.pending(), 0u);
}

TEST_F(ICMessageQueueTest, ClearResetsLingerTimer) {
    queue.enqueue(make_msg("Phoenix", "First"));
    queue.next();
    queue.enqueue(make_msg("Edgeworth", "Second"));

    // Accumulate some linger time but not enough
    queue.tick(200, true);

    queue.clear();

    // Re-enqueue and verify linger timer was reset
    queue.enqueue(make_msg("Maya", "Fresh"));
    queue.next();
    queue.enqueue(make_msg("Godot", "AfterClear"));

    // Should need full 300ms, not just the remaining 100ms
    EXPECT_FALSE(queue.tick(100, true));
    EXPECT_FALSE(queue.tick(100, true));
    EXPECT_TRUE(queue.tick(100, true));
}

// ---------------------------------------------------------------------------
// Prefetch callback
// ---------------------------------------------------------------------------

TEST_F(ICMessageQueueTest, PrefetchCalledWhenEnqueuedWhilePlaying) {
    std::vector<std::string> prefetched;
    queue.set_prefetch([&](const ICMessage& msg) {
        prefetched.push_back(msg.character);
    });

    queue.enqueue(make_msg("Phoenix", "First"));
    queue.next(); // start playing

    // Enqueue while playing should trigger prefetch
    queue.enqueue(make_msg("Edgeworth", "Second"));
    queue.enqueue(make_msg("Maya", "Third"));

    ASSERT_EQ(prefetched.size(), 2u);
    EXPECT_EQ(prefetched[0], "Edgeworth");
    EXPECT_EQ(prefetched[1], "Maya");
}

TEST_F(ICMessageQueueTest, PrefetchNotCalledWhenQueueIsIdle) {
    int prefetch_count = 0;
    queue.set_prefetch([&](const ICMessage&) {
        prefetch_count++;
    });

    // Nothing is playing, so no prefetch needed
    queue.enqueue(make_msg("Phoenix", "First"));
    EXPECT_EQ(prefetch_count, 0);
}

TEST_F(ICMessageQueueTest, PrefetchNotCalledWhenNoPrefetchSet) {
    // No prefetch set, should not crash
    queue.enqueue(make_msg("Phoenix", "First"));
    queue.next();
    queue.enqueue(make_msg("Edgeworth", "Second"));
    EXPECT_EQ(queue.pending(), 1u);
}

TEST_F(ICMessageQueueTest, PrefetchReceivesCorrectMessageData) {
    ICMessage captured;
    bool called = false;
    queue.set_prefetch([&](const ICMessage& msg) {
        captured = msg;
        called = true;
    });

    queue.enqueue(make_msg("Phoenix", "First"));
    queue.next();

    ICMessage second = make_msg("Edgeworth", "Second");
    second.emote = "thinking";
    second.side = "pro";
    queue.enqueue(second);

    ASSERT_TRUE(called);
    EXPECT_EQ(captured.character, "Edgeworth");
    EXPECT_EQ(captured.message, "Second");
    EXPECT_EQ(captured.emote, "thinking");
    EXPECT_EQ(captured.side, "pro");
}

// ---------------------------------------------------------------------------
// ICMessage::is_objection()
// ---------------------------------------------------------------------------

TEST_F(ICMessageQueueTest, IsObjectionReturnsFalseForZero) {
    ICMessage msg = make_msg();
    msg.objection_mod = 0;
    EXPECT_FALSE(msg.is_objection());
}

TEST_F(ICMessageQueueTest, IsObjectionReturnsTrueForOne) {
    ICMessage msg = make_msg();
    msg.objection_mod = 1;
    EXPECT_TRUE(msg.is_objection());
}

TEST_F(ICMessageQueueTest, IsObjectionReturnsTrueForTwo) {
    ICMessage msg = make_msg();
    msg.objection_mod = 2;
    EXPECT_TRUE(msg.is_objection());
}

TEST_F(ICMessageQueueTest, IsObjectionReturnsTrueForThree) {
    ICMessage msg = make_msg();
    msg.objection_mod = 3;
    EXPECT_TRUE(msg.is_objection());
}

TEST_F(ICMessageQueueTest, IsObjectionReturnsTrueForFour) {
    ICMessage msg = make_msg();
    msg.objection_mod = 4;
    EXPECT_TRUE(msg.is_objection());
}

TEST_F(ICMessageQueueTest, IsObjectionReturnsFalseForNegative) {
    ICMessage msg = make_msg();
    msg.objection_mod = -1;
    EXPECT_FALSE(msg.is_objection());
}

TEST_F(ICMessageQueueTest, IsObjectionReturnsFalseForFive) {
    ICMessage msg = make_msg();
    msg.objection_mod = 5;
    EXPECT_FALSE(msg.is_objection());
}

// ---------------------------------------------------------------------------
// Objection behavior
// ---------------------------------------------------------------------------

TEST_F(ICMessageQueueTest, ObjectionClearsQueueAndPlaysImmediately) {
    queue.enqueue(make_msg("Phoenix", "First"));
    queue.next(); // playing first
    queue.enqueue(make_msg("Edgeworth", "Second"));
    queue.enqueue(make_msg("Maya", "Third"));
    EXPECT_EQ(queue.pending(), 2u);

    // Objection should clear queue and be immediately ready
    queue.enqueue(make_objection("Godot", 1));
    EXPECT_EQ(queue.pending(), 1u);
    EXPECT_FALSE(queue.is_playing()); // objection resets playing

    auto msg = queue.next();
    ASSERT_TRUE(msg.has_value());
    EXPECT_EQ(msg->character, "Godot");
    EXPECT_EQ(msg->objection_mod, 1);
}

TEST_F(ICMessageQueueTest, ObjectionResetsLingerTimer) {
    queue.enqueue(make_msg("Phoenix", "First"));
    queue.next();
    queue.enqueue(make_msg("Edgeworth", "Second"));

    // Accumulate some linger
    queue.tick(200, true);

    // Objection interrupts
    queue.enqueue(make_objection("Judge", 2));

    auto msg = queue.next();
    ASSERT_TRUE(msg.has_value());
    EXPECT_EQ(msg->character, "Judge");
}

TEST_F(ICMessageQueueTest, ObjectionModTwoIsObjection) {
    queue.enqueue(make_objection("Phoenix", 2));
    auto msg = queue.next();
    ASSERT_TRUE(msg.has_value());
    EXPECT_TRUE(msg->is_objection());
}

TEST_F(ICMessageQueueTest, ObjectionModFourIsObjection) {
    queue.enqueue(make_objection("Phoenix", 4));
    auto msg = queue.next();
    ASSERT_TRUE(msg.has_value());
    EXPECT_TRUE(msg->is_objection());
}

// ---------------------------------------------------------------------------
// Linger timer behavior
// ---------------------------------------------------------------------------

TEST_F(ICMessageQueueTest, LingerTimerRequiresFullDuration) {
    queue.enqueue(make_msg("Phoenix", "First"));
    queue.next();
    queue.enqueue(make_msg("Edgeworth", "Second"));

    // Tick with small deltas - should not advance until 300ms total
    EXPECT_FALSE(queue.tick(100, true));
    EXPECT_FALSE(queue.tick(100, true));
    // At 200ms, still not ready
    EXPECT_TRUE(queue.is_playing());

    // At 300ms total, should advance
    EXPECT_TRUE(queue.tick(100, true));
    EXPECT_FALSE(queue.is_playing());
}

TEST_F(ICMessageQueueTest, LingerTimerDoesNotAccumulateWhenNotDone) {
    queue.enqueue(make_msg("Phoenix", "First"));
    queue.next();
    queue.enqueue(make_msg("Edgeworth", "Second"));

    // Message not done - linger should not accumulate
    EXPECT_FALSE(queue.tick(200, false));
    EXPECT_FALSE(queue.tick(200, false));

    // Now message is done - need full 300ms from scratch
    EXPECT_FALSE(queue.tick(100, true));
    EXPECT_FALSE(queue.tick(100, true));
    EXPECT_TRUE(queue.tick(100, true));
}

TEST_F(ICMessageQueueTest, LingerTimerResetsAfterNextMessage) {
    queue.enqueue(make_msg("Phoenix", "First"));
    queue.enqueue(make_msg("Edgeworth", "Second"));
    queue.enqueue(make_msg("Maya", "Third"));

    queue.next(); // play first

    // Complete first message
    queue.tick(300, true);
    queue.next(); // play second

    // Second message needs its own full linger
    EXPECT_FALSE(queue.tick(100, true));
    EXPECT_FALSE(queue.tick(100, true));
    EXPECT_TRUE(queue.tick(100, true));

    auto msg = queue.next();
    ASSERT_TRUE(msg.has_value());
    EXPECT_EQ(msg->character, "Maya");
}

TEST_F(ICMessageQueueTest, LingerTimerAccumulatesAcrossMultipleTicks) {
    queue.enqueue(make_msg("Phoenix", "First"));
    queue.next();
    queue.enqueue(make_msg("Edgeworth", "Second"));

    // Accumulate in 10ms increments: 29 ticks = 290ms, still under 300ms
    for (int i = 0; i < 29; i++) {
        EXPECT_FALSE(queue.tick(10, true));
    }
    // 30th tick: 300ms total, hits LINGER_DURATION_MS exactly
    EXPECT_TRUE(queue.tick(10, true));
}

TEST_F(ICMessageQueueTest, LingerExactly300msTriggersTransition) {
    queue.enqueue(make_msg("Phoenix", "First"));
    queue.next();
    queue.enqueue(make_msg("Edgeworth", "Second"));

    // Exactly 299ms should not trigger
    EXPECT_FALSE(queue.tick(299, true));
    // One more ms to hit 300 exactly
    EXPECT_TRUE(queue.tick(1, true));
}

TEST_F(ICMessageQueueTest, LingerSingleLargeTickTriggersTransition) {
    queue.enqueue(make_msg("Phoenix", "First"));
    queue.next();
    queue.enqueue(make_msg("Edgeworth", "Second"));

    // Single tick with more than 300ms
    EXPECT_TRUE(queue.tick(500, true));
}

TEST_F(ICMessageQueueTest, NoLingerWhenQueueEmpty) {
    queue.enqueue(make_msg("Phoenix", "First"));
    queue.next();

    // No queued messages, tick should return false even with done=true
    EXPECT_FALSE(queue.tick(300, true));
    EXPECT_FALSE(queue.tick(300, true));
}

// ---------------------------------------------------------------------------
// Edge cases
// ---------------------------------------------------------------------------

TEST_F(ICMessageQueueTest, EnqueueAfterClearWorksNormally) {
    queue.enqueue(make_msg("Phoenix", "First"));
    queue.next();
    queue.clear();

    queue.enqueue(make_msg("Maya", "AfterClear"));
    auto msg = queue.next();
    ASSERT_TRUE(msg.has_value());
    EXPECT_EQ(msg->character, "Maya");
}

TEST_F(ICMessageQueueTest, MultipleNextCallsReturnNulloptAfterFirst) {
    queue.enqueue(make_msg("Phoenix", "Only"));
    auto first = queue.next();
    ASSERT_TRUE(first.has_value());

    auto second = queue.next();
    EXPECT_FALSE(second.has_value());

    auto third = queue.next();
    EXPECT_FALSE(third.has_value());
}

TEST_F(ICMessageQueueTest, ObjectionOnEmptyQueueWorks) {
    queue.enqueue(make_objection("Phoenix", 3));
    EXPECT_EQ(queue.pending(), 1u);

    auto msg = queue.next();
    ASSERT_TRUE(msg.has_value());
    EXPECT_EQ(msg->character, "Phoenix");
    EXPECT_TRUE(msg->is_objection());
}

TEST_F(ICMessageQueueTest, PrefetchNotCalledForObjections) {
    int prefetch_count = 0;
    queue.set_prefetch([&](const ICMessage&) {
        prefetch_count++;
    });

    queue.enqueue(make_msg("Phoenix", "First"));
    queue.next(); // playing

    // Objection clears playing_ before the prefetch check in enqueue
    // so prefetch should not be called (playing_ is set to false)
    queue.enqueue(make_objection("Edgeworth", 1));

    // The objection code sets playing_ = false before the message
    // is added, so the prefetch guard (if playing_ && prefetch_) won't fire.
    // But actually, the objection path returns early before the normal
    // enqueue path, so prefetch is never called.
    EXPECT_EQ(prefetch_count, 0);
}

TEST_F(ICMessageQueueTest, RapidEnqueueAndDrainCycle) {
    // Simulate a rapid fire chat scenario
    for (int i = 0; i < 10; i++) {
        queue.enqueue(make_msg("Speaker" + std::to_string(i), "Msg" + std::to_string(i)));
    }
    EXPECT_EQ(queue.pending(), 10u);

    for (int i = 0; i < 10; i++) {
        auto msg = queue.next();
        ASSERT_TRUE(msg.has_value());
        EXPECT_EQ(msg->character, "Speaker" + std::to_string(i));

        if (i < 9) {
            // Linger and advance to next
            queue.tick(300, true);
        }
    }

    EXPECT_EQ(queue.pending(), 0u);
}
