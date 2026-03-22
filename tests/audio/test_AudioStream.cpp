#include "audio/AudioStream.h"

#include <gtest/gtest.h>
#include <algorithm>
#include <array>
#include <cstring>
#include <thread>
#include <vector>

// ===========================================================================
// PcmRingBuffer tests
// ===========================================================================

TEST(PcmRingBuffer, DefaultEmpty) {
    PcmRingBuffer ring(1024);
    EXPECT_EQ(ring.available(), 0u);
}

TEST(PcmRingBuffer, WriteAndRead) {
    PcmRingBuffer ring(1024);
    std::vector<float> in = {1.0f, 2.0f, 3.0f, 4.0f};
    size_t written = ring.write(in.data(), in.size());
    EXPECT_EQ(written, 4u);
    EXPECT_EQ(ring.available(), 4u);

    std::vector<float> out(4, 0.0f);
    size_t got = ring.read(out.data(), out.size());
    EXPECT_EQ(got, 4u);
    EXPECT_EQ(ring.available(), 0u);
    EXPECT_EQ(out, in);
}

TEST(PcmRingBuffer, ReadReturnsLessWhenEmpty) {
    PcmRingBuffer ring(1024);
    std::vector<float> in = {10.0f, 20.0f};
    ring.write(in.data(), in.size());

    std::vector<float> out(8, 0.0f);
    size_t got = ring.read(out.data(), out.size());
    EXPECT_EQ(got, 2u);
    EXPECT_FLOAT_EQ(out[0], 10.0f);
    EXPECT_FLOAT_EQ(out[1], 20.0f);
}

TEST(PcmRingBuffer, WriteReturnsLessWhenFull) {
    // Capacity is rounded to next power of 2, so requesting 4 gives 4.
    PcmRingBuffer ring(4);
    std::vector<float> in = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f};
    size_t written = ring.write(in.data(), in.size());
    // Ring buffer capacity is 4, so at most 4 samples can be stored.
    EXPECT_LE(written, 4u);
    EXPECT_EQ(ring.available(), written);
}

TEST(PcmRingBuffer, ResetClearsBuffer) {
    PcmRingBuffer ring(1024);
    std::vector<float> in = {1.0f, 2.0f, 3.0f};
    ring.write(in.data(), in.size());
    EXPECT_EQ(ring.available(), 3u);

    ring.reset();
    EXPECT_EQ(ring.available(), 0u);
}

TEST(PcmRingBuffer, WrapAround) {
    // Use a small power-of-2 ring.
    PcmRingBuffer ring(8);
    std::vector<float> in1 = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f};
    ring.write(in1.data(), in1.size());

    // Read some out to advance read head.
    std::vector<float> tmp(4);
    ring.read(tmp.data(), tmp.size());
    EXPECT_FLOAT_EQ(tmp[0], 1.0f);
    EXPECT_FLOAT_EQ(tmp[3], 4.0f);

    // Now write more, causing wrap-around.
    std::vector<float> in2 = {7.0f, 8.0f, 9.0f, 10.0f};
    size_t w = ring.write(in2.data(), in2.size());
    EXPECT_GT(w, 0u);

    // Read everything remaining.
    std::vector<float> out(ring.available());
    size_t got = ring.read(out.data(), out.size());
    EXPECT_EQ(got, out.size());
    // First two should be leftovers from in1, then whatever wrapped from in2.
    EXPECT_FLOAT_EQ(out[0], 5.0f);
    EXPECT_FLOAT_EQ(out[1], 6.0f);
}

TEST(PcmRingBuffer, MultipleWriteReadCycles) {
    PcmRingBuffer ring(256);
    for (int cycle = 0; cycle < 50; ++cycle) {
        float val = static_cast<float>(cycle);
        std::vector<float> in(3, val);
        size_t w = ring.write(in.data(), in.size());
        EXPECT_EQ(w, 3u);

        std::vector<float> out(3);
        size_t r = ring.read(out.data(), out.size());
        EXPECT_EQ(r, 3u);
        for (auto& v : out)
            EXPECT_FLOAT_EQ(v, val);
    }
}

// ===========================================================================
// AudioStream — default / initial state
// ===========================================================================

TEST(AudioStream, DefaultState) {
    AudioStream stream;
    EXPECT_FALSE(stream.is_ready());
    EXPECT_FALSE(stream.is_finished());
    EXPECT_FALSE(stream.is_cancelled());
    EXPECT_FALSE(stream.is_looping());
    EXPECT_EQ(stream.sample_rate(), 48000u);
    EXPECT_EQ(stream.channels(), 2u);
    EXPECT_EQ(stream.buffered_samples(), 0u);
    EXPECT_EQ(stream.loop_start(), 0);
    EXPECT_EQ(stream.loop_end(), 0);
}

// ===========================================================================
// AudioStream — feed() and raw buffer
// ===========================================================================

TEST(AudioStream, FeedAppendsData) {
    AudioStream stream;
    std::vector<uint8_t> data = {0x01, 0x02, 0x03, 0x04};
    stream.feed(data.data(), data.size());
    // Verify position is at start and data is readable via stream_tell/stream_read.
    EXPECT_EQ(stream.stream_tell(), 0);

    uint8_t buf[4] = {};
    int got = stream.stream_read(buf, 4);
    EXPECT_EQ(got, 4);
    EXPECT_EQ(buf[0], 0x01);
    EXPECT_EQ(buf[3], 0x04);
    EXPECT_EQ(stream.stream_tell(), 4);
}

TEST(AudioStream, FeedMultipleChunks) {
    AudioStream stream;
    std::vector<uint8_t> chunk1 = {0xAA, 0xBB};
    std::vector<uint8_t> chunk2 = {0xCC, 0xDD, 0xEE};
    stream.feed(chunk1.data(), chunk1.size());
    stream.feed(chunk2.data(), chunk2.size());

    uint8_t buf[5] = {};
    int got = stream.stream_read(buf, 5);
    EXPECT_EQ(got, 5);
    EXPECT_EQ(buf[0], 0xAA);
    EXPECT_EQ(buf[1], 0xBB);
    EXPECT_EQ(buf[2], 0xCC);
    EXPECT_EQ(buf[3], 0xDD);
    EXPECT_EQ(buf[4], 0xEE);
}

TEST(AudioStream, FeedEmptyData) {
    AudioStream stream;
    stream.feed(nullptr, 0);
    EXPECT_EQ(stream.stream_tell(), 0);
}

// ===========================================================================
// AudioStream — mark_complete()
// ===========================================================================

TEST(AudioStream, MarkCompleteWithEmptyData) {
    AudioStream stream;
    // No data fed — mark_complete should set finished since there's nothing to decode.
    stream.mark_complete();
    EXPECT_TRUE(stream.is_finished());
}

TEST(AudioStream, MarkCompleteEnablesSeekEnd) {
    AudioStream stream;
    std::vector<uint8_t> data = {0x01, 0x02, 0x03, 0x04, 0x05};
    stream.feed(data.data(), data.size());

    // Before mark_complete, SEEK_END should fail.
    int result = stream.stream_seek(0, SEEK_END);
    EXPECT_EQ(result, -1);

    stream.cancel(); // Prevent decode thread from launching.
    stream.mark_complete();

    // After mark_complete, SEEK_END should work.
    result = stream.stream_seek(0, SEEK_END);
    EXPECT_EQ(result, 0);
    EXPECT_EQ(stream.stream_tell(), 5);
}

// ===========================================================================
// AudioStream — cancel() / is_cancelled()
// ===========================================================================

TEST(AudioStream, CancelSetsFlag) {
    AudioStream stream;
    EXPECT_FALSE(stream.is_cancelled());
    stream.cancel();
    EXPECT_TRUE(stream.is_cancelled());
}

TEST(AudioStream, CancelMakesStreamReadReturnZero) {
    AudioStream stream;
    std::vector<uint8_t> data = {0x01, 0x02, 0x03};
    stream.feed(data.data(), data.size());

    stream.cancel();

    uint8_t buf[3] = {};
    int got = stream.stream_read(buf, 3);
    EXPECT_EQ(got, 0);
}

TEST(AudioStream, CancelIdempotent) {
    AudioStream stream;
    stream.cancel();
    stream.cancel();
    stream.cancel();
    EXPECT_TRUE(stream.is_cancelled());
}

// ===========================================================================
// AudioStream — set_looping() / is_looping()
// ===========================================================================

TEST(AudioStream, LoopingDefaultFalse) {
    AudioStream stream;
    EXPECT_FALSE(stream.is_looping());
}

TEST(AudioStream, SetLoopingTrue) {
    AudioStream stream;
    stream.set_looping(true);
    EXPECT_TRUE(stream.is_looping());
}

TEST(AudioStream, SetLoopingToggle) {
    AudioStream stream;
    stream.set_looping(true);
    EXPECT_TRUE(stream.is_looping());
    stream.set_looping(false);
    EXPECT_FALSE(stream.is_looping());
    stream.set_looping(true);
    EXPECT_TRUE(stream.is_looping());
}

// ===========================================================================
// AudioStream — set_loop_points()
// ===========================================================================

TEST(AudioStream, LoopPointsDefault) {
    AudioStream stream;
    EXPECT_EQ(stream.loop_start(), 0);
    EXPECT_EQ(stream.loop_end(), 0);
}

TEST(AudioStream, SetLoopPointsBothValues) {
    AudioStream stream;
    stream.set_loop_points(48000, 96000);
    EXPECT_EQ(stream.loop_start(), 48000);
    EXPECT_EQ(stream.loop_end(), 96000);
}

TEST(AudioStream, SetLoopPointsStartOnly) {
    AudioStream stream;
    stream.set_loop_points(12345);
    EXPECT_EQ(stream.loop_start(), 12345);
    EXPECT_EQ(stream.loop_end(), 0);  // Default: loops to end of stream.
}

TEST(AudioStream, SetLoopPointsOverwrite) {
    AudioStream stream;
    stream.set_loop_points(100, 200);
    stream.set_loop_points(300, 400);
    EXPECT_EQ(stream.loop_start(), 300);
    EXPECT_EQ(stream.loop_end(), 400);
}

TEST(AudioStream, SetLoopPointsZeroEndMeansFullStream) {
    AudioStream stream;
    stream.set_loop_points(48000, 0);
    EXPECT_EQ(stream.loop_start(), 48000);
    EXPECT_EQ(stream.loop_end(), 0);  // 0 = end of stream per the API.
}

// ===========================================================================
// AudioStream — stream_seek / stream_tell (raw buffer management)
// ===========================================================================

TEST(AudioStream, SeekSetBasic) {
    AudioStream stream;
    std::vector<uint8_t> data(10, 0xFF);
    stream.feed(data.data(), data.size());

    EXPECT_EQ(stream.stream_seek(5, SEEK_SET), 0);
    EXPECT_EQ(stream.stream_tell(), 5);
}

TEST(AudioStream, SeekCurForward) {
    AudioStream stream;
    std::vector<uint8_t> data(20, 0xAA);
    stream.feed(data.data(), data.size());

    stream.stream_seek(5, SEEK_SET);
    EXPECT_EQ(stream.stream_seek(3, SEEK_CUR), 0);
    EXPECT_EQ(stream.stream_tell(), 8);
}

TEST(AudioStream, SeekCurBackward) {
    AudioStream stream;
    std::vector<uint8_t> data(20, 0xAA);
    stream.feed(data.data(), data.size());

    stream.stream_seek(10, SEEK_SET);
    EXPECT_EQ(stream.stream_seek(-4, SEEK_CUR), 0);
    EXPECT_EQ(stream.stream_tell(), 6);
}

TEST(AudioStream, SeekSetNegativeFails) {
    AudioStream stream;
    std::vector<uint8_t> data(10, 0x00);
    stream.feed(data.data(), data.size());

    EXPECT_EQ(stream.stream_seek(-1, SEEK_SET), -1);
    EXPECT_EQ(stream.stream_tell(), 0);  // Position unchanged.
}

TEST(AudioStream, SeekEndAfterComplete) {
    AudioStream stream;
    std::vector<uint8_t> data(100, 0x42);
    stream.feed(data.data(), data.size());
    stream.cancel();
    stream.mark_complete();

    // SEEK_END with offset 0 goes to end.
    EXPECT_EQ(stream.stream_seek(0, SEEK_END), 0);
    EXPECT_EQ(stream.stream_tell(), 100);

    // SEEK_END with negative offset goes back from end.
    EXPECT_EQ(stream.stream_seek(-10, SEEK_END), 0);
    EXPECT_EQ(stream.stream_tell(), 90);
}

TEST(AudioStream, SeekEndBeforeCompleteFails) {
    AudioStream stream;
    std::vector<uint8_t> data(50, 0x00);
    stream.feed(data.data(), data.size());
    // raw_complete_ is still false.
    EXPECT_EQ(stream.stream_seek(0, SEEK_END), -1);
}

TEST(AudioStream, SeekInvalidWhenceFails) {
    AudioStream stream;
    std::vector<uint8_t> data(10, 0x00);
    stream.feed(data.data(), data.size());

    EXPECT_EQ(stream.stream_seek(0, 42), -1);
}

TEST(AudioStream, StreamReadPartial) {
    AudioStream stream;
    std::vector<uint8_t> data = {0x10, 0x20, 0x30, 0x40, 0x50};
    stream.feed(data.data(), data.size());

    // Read only 2 bytes.
    uint8_t buf[2] = {};
    int got = stream.stream_read(buf, 2);
    EXPECT_EQ(got, 2);
    EXPECT_EQ(buf[0], 0x10);
    EXPECT_EQ(buf[1], 0x20);
    EXPECT_EQ(stream.stream_tell(), 2);

    // Read remaining.
    uint8_t buf2[3] = {};
    got = stream.stream_read(buf2, 3);
    EXPECT_EQ(got, 3);
    EXPECT_EQ(buf2[0], 0x30);
    EXPECT_EQ(buf2[2], 0x50);
    EXPECT_EQ(stream.stream_tell(), 5);
}

TEST(AudioStream, SeekThenRead) {
    AudioStream stream;
    std::vector<uint8_t> data = {0x00, 0x11, 0x22, 0x33, 0x44};
    stream.feed(data.data(), data.size());

    stream.stream_seek(2, SEEK_SET);
    uint8_t buf[2] = {};
    int got = stream.stream_read(buf, 2);
    EXPECT_EQ(got, 2);
    EXPECT_EQ(buf[0], 0x22);
    EXPECT_EQ(buf[1], 0x33);
}

// ===========================================================================
// AudioStream — read_frames on empty buffer
// ===========================================================================

TEST(AudioStream, ReadFramesReturnsZeroWhenEmpty) {
    AudioStream stream;
    float output[64] = {};
    int frames = stream.read_frames(output, 32);
    EXPECT_EQ(frames, 0);
}

// ===========================================================================
// AudioStream — sample_rate / channels accessors
// ===========================================================================

TEST(AudioStream, SampleRateIs48000) {
    AudioStream stream;
    EXPECT_EQ(stream.sample_rate(), 48000u);
}

TEST(AudioStream, ChannelsIs2) {
    AudioStream stream;
    EXPECT_EQ(stream.channels(), 2u);
}

// ===========================================================================
// AudioStream — destructor cancels cleanly
// ===========================================================================

TEST(AudioStream, DestructorDoesNotHang) {
    // Verifies that destroying an AudioStream with fed data doesn't hang.
    {
        AudioStream stream;
        std::vector<uint8_t> data(1024, 0x00);
        stream.feed(data.data(), data.size());
    }
    // If we reach here, the destructor completed without hanging.
    SUCCEED();
}

TEST(AudioStream, DestructorAfterMarkCompleteDoesNotHang) {
    // Feeds invalid (non-Opus) data, marks complete, and destroys.
    // The decode thread should fail to open the Opus file and exit.
    {
        AudioStream stream;
        std::vector<uint8_t> garbage(256, 0xFF);
        stream.feed(garbage.data(), garbage.size());
        stream.mark_complete();
        // Give decode thread a moment to start and fail.
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    SUCCEED();
}

// ===========================================================================
// AudioStream — thread safety of set/get looping
// ===========================================================================

TEST(AudioStream, ConcurrentLoopingAccess) {
    AudioStream stream;

    // Hammer set_looping / is_looping from two threads.
    std::atomic<bool> done{false};
    std::thread writer([&] {
        for (int i = 0; i < 1000; ++i) {
            stream.set_looping(i % 2 == 0);
        }
        done.store(true);
    });

    while (!done.load()) {
        // Just read — should not crash or UB.
        [[maybe_unused]] bool val = stream.is_looping();
    }

    writer.join();
}

TEST(AudioStream, ConcurrentLoopPointAccess) {
    AudioStream stream;

    std::thread writer([&] {
        for (int i = 0; i < 1000; ++i) {
            stream.set_loop_points(i * 100, i * 200);
        }
    });

    for (int i = 0; i < 1000; ++i) {
        [[maybe_unused]] auto s = stream.loop_start();
        [[maybe_unused]] auto e = stream.loop_end();
    }

    writer.join();
}
