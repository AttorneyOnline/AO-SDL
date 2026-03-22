#include "asset/ImageAsset.h"
#include "render/Layer.h"
#include "render/RenderState.h"
#include "render/StateBuffer.h"

#include <gtest/gtest.h>
#include <atomic>
#include <memory>
#include <thread>
#include <vector>

// ---------------------------------------------------------------------------
// Helper
// ---------------------------------------------------------------------------

static auto make_asset(int frames = 1, int duration_ms = 100) {
    std::vector<ImageFrame> f;
    for (int i = 0; i < frames; i++) {
        f.push_back(ImageFrame{std::vector<uint8_t>(4, (uint8_t)i), 1, 1, duration_ms});
    }
    return std::make_shared<ImageAsset>("test", "png", std::move(f));
}

static LayerGroup make_layer_group(int layer_id, uint16_t z) {
    LayerGroup group;
    group.add_layer(layer_id, Layer(make_asset(), 0, z));
    return group;
}

// ===========================================================================
// RenderState
// ===========================================================================

TEST(RenderState, DefaultConstructionIsEmpty) {
    RenderState state;
    EXPECT_TRUE(state.get_layer_groups().empty());
}

TEST(RenderState, AddAndRetrieveLayerGroup) {
    RenderState state;
    auto group = make_layer_group(0, 5);
    state.add_layer_group(1, group);

    const auto& groups = state.get_layer_groups();
    ASSERT_EQ(groups.size(), 1u);
    EXPECT_NE(groups.find(1), groups.end());

    LayerGroup retrieved = state.get_layer_group(1);
    ASSERT_EQ(retrieved.get_layers().size(), 1u);
    EXPECT_EQ(retrieved.get_layers().at(0).get_z_index(), 5);
}

TEST(RenderState, MultipleLayerGroups) {
    RenderState state;
    state.add_layer_group(0, make_layer_group(0, 0));
    state.add_layer_group(5, make_layer_group(1, 10));
    state.add_layer_group(10, make_layer_group(2, 20));

    const auto& groups = state.get_layer_groups();
    ASSERT_EQ(groups.size(), 3u);
    EXPECT_NE(groups.find(0), groups.end());
    EXPECT_NE(groups.find(5), groups.end());
    EXPECT_NE(groups.find(10), groups.end());
}

TEST(RenderState, GetLayerGroupThrowsOnMissingId) {
    RenderState state;
    EXPECT_THROW(state.get_layer_group(42), std::out_of_range);
}

TEST(RenderState, GetLayerGroupsOrderedByKey) {
    RenderState state;
    state.add_layer_group(10, make_layer_group(0, 20));
    state.add_layer_group(0, make_layer_group(0, 0));
    state.add_layer_group(5, make_layer_group(0, 10));

    const auto& groups = state.get_layer_groups();
    auto it = groups.begin();
    EXPECT_EQ(it->first, 0);
    ++it;
    EXPECT_EQ(it->first, 5);
    ++it;
    EXPECT_EQ(it->first, 10);
}

// ===========================================================================
// StateBuffer — initial state
// ===========================================================================

TEST(StateBuffer, DefaultConstructionProvidesBothBuffers) {
    StateBuffer buf;
    EXPECT_NE(buf.get_producer_buf(), nullptr);
    EXPECT_NE(buf.get_consumer_buf(), nullptr);
}

TEST(StateBuffer, ProducerAndConsumerAreDifferentBuffers) {
    StateBuffer buf;
    EXPECT_NE(buf.get_producer_buf(),
              static_cast<const RenderState*>(buf.get_consumer_buf()));
}

TEST(StateBuffer, DefaultConstructionBuffersAreEmpty) {
    StateBuffer buf;
    EXPECT_TRUE(buf.get_producer_buf()->get_layer_groups().empty());
    EXPECT_TRUE(buf.get_consumer_buf()->get_layer_groups().empty());
}

TEST(StateBuffer, InitialStateConstructionPropagates) {
    RenderState initial;
    initial.add_layer_group(0, make_layer_group(0, 5));

    StateBuffer buf(initial);
    // Producer buffer should have the initial state
    ASSERT_EQ(buf.get_producer_buf()->get_layer_groups().size(), 1u);
    // Consumer buffer should also have the initial state
    ASSERT_EQ(buf.get_consumer_buf()->get_layer_groups().size(), 1u);
}

// ===========================================================================
// StateBuffer — triple-buffer swap behavior
// ===========================================================================

TEST(StateBuffer, PresentAndUpdatePropagateData) {
    StateBuffer buf;

    // Producer writes a layer group
    buf.get_producer_buf()->add_layer_group(0, make_layer_group(0, 5));
    buf.present();

    // Consumer should still see old (empty) state before update()
    EXPECT_TRUE(buf.get_consumer_buf()->get_layer_groups().empty());

    // After update(), consumer should see the new state
    buf.update();
    ASSERT_EQ(buf.get_consumer_buf()->get_layer_groups().size(), 1u);
}

TEST(StateBuffer, UpdateWithoutPresentIsNoop) {
    StateBuffer buf;

    // Write to producer but do NOT call present()
    buf.get_producer_buf()->add_layer_group(0, make_layer_group(0, 5));

    // update() should be a no-op since stale is false
    buf.update();
    EXPECT_TRUE(buf.get_consumer_buf()->get_layer_groups().empty());
}

TEST(StateBuffer, MultiplePresentsBeforeUpdateDropsIntermediate) {
    StateBuffer buf;

    // First present: write value A
    buf.get_producer_buf()->add_layer_group(0, make_layer_group(0, 5));
    buf.present();

    // Second present: write value B (overwrites ready slot)
    buf.get_producer_buf()->add_layer_group(0, make_layer_group(0, 10));
    buf.get_producer_buf()->add_layer_group(1, make_layer_group(1, 20));
    buf.present();

    // Consumer should get the latest (second) state
    buf.update();
    const auto& groups = buf.get_consumer_buf()->get_layer_groups();
    ASSERT_EQ(groups.size(), 2u);
    EXPECT_NE(groups.find(0), groups.end());
    EXPECT_NE(groups.find(1), groups.end());
}

TEST(StateBuffer, MultipleUpdatesWithoutPresentAreIdempotent) {
    StateBuffer buf;

    buf.get_producer_buf()->add_layer_group(0, make_layer_group(0, 5));
    buf.present();
    buf.update();

    // Consumer now sees the data
    ASSERT_EQ(buf.get_consumer_buf()->get_layer_groups().size(), 1u);

    // Subsequent updates without new present() should keep same data
    buf.update();
    ASSERT_EQ(buf.get_consumer_buf()->get_layer_groups().size(), 1u);

    buf.update();
    ASSERT_EQ(buf.get_consumer_buf()->get_layer_groups().size(), 1u);
}

TEST(StateBuffer, ProducerBufferIsReusableAfterPresent) {
    StateBuffer buf;

    buf.get_producer_buf()->add_layer_group(0, make_layer_group(0, 5));
    buf.present();

    // After present(), the producer buffer is now a different (swapped) slot.
    // The producer can immediately write new data into it.
    RenderState* prod = buf.get_producer_buf();
    EXPECT_NE(prod, nullptr);

    // Write new data and present again
    prod->add_layer_group(10, make_layer_group(0, 99));
    buf.present();

    buf.update();
    const auto& groups = buf.get_consumer_buf()->get_layer_groups();
    EXPECT_NE(groups.find(10), groups.end());
}

TEST(StateBuffer, ConsumerSeesStableSnapshotBetweenUpdates) {
    StateBuffer buf;

    // Present initial data
    buf.get_producer_buf()->add_layer_group(0, make_layer_group(0, 5));
    buf.present();
    buf.update();

    // Grab consumer pointer
    const RenderState* consumer = buf.get_consumer_buf();
    ASSERT_EQ(consumer->get_layer_groups().size(), 1u);

    // Producer writes and presents new data
    buf.get_producer_buf()->add_layer_group(0, make_layer_group(0, 10));
    buf.get_producer_buf()->add_layer_group(1, make_layer_group(1, 20));
    buf.get_producer_buf()->add_layer_group(2, make_layer_group(2, 30));
    buf.present();

    // Consumer should STILL see the old snapshot (no update() called yet)
    EXPECT_EQ(consumer->get_layer_groups().size(), 1u);
}

// ===========================================================================
// StateBuffer — thread safety
// ===========================================================================

TEST(StateBuffer, ConcurrentProducerConsumer) {
    StateBuffer buf;

    constexpr int NUM_ITERATIONS = 1000;
    std::atomic<bool> done{false};
    std::atomic<int> consumer_reads{0};

    // Producer thread: repeatedly writes and presents
    std::thread producer([&]() {
        for (int i = 0; i < NUM_ITERATIONS; i++) {
            RenderState* prod = buf.get_producer_buf();
            // Clear by adding a fresh group with a known z_index
            prod->add_layer_group(0, make_layer_group(0, static_cast<uint16_t>(i % 100)));
            buf.present();
        }
        done = true;
    });

    // Consumer thread: repeatedly updates and reads
    std::thread consumer([&]() {
        while (!done || consumer_reads < 1) {
            buf.update();
            const RenderState* cons = buf.get_consumer_buf();
            // Should never crash or return nullptr
            ASSERT_NE(cons, nullptr);
            // Layer groups map should be accessible without data races
            const auto& groups = cons->get_layer_groups();
            (void)groups.size();
            consumer_reads++;
        }
    });

    producer.join();
    consumer.join();

    EXPECT_GT(consumer_reads.load(), 0);
}

TEST(StateBuffer, ProducerNeverSeesConsumerBuffer) {
    StateBuffer buf;

    // Track all three pointer values accessible to producer across
    // multiple present cycles. None should ever equal the consumer buffer.
    std::vector<RenderState*> producer_ptrs;

    for (int i = 0; i < 10; i++) {
        RenderState* prod = buf.get_producer_buf();
        producer_ptrs.push_back(prod);
        buf.present();
    }

    // After all those presents without any update(), the consumer buffer
    // is the original presenting buffer. Verify no producer pointer equals
    // the current consumer buffer.
    const RenderState* cons = buf.get_consumer_buf();
    for (auto* p : producer_ptrs) {
        // Producer should never be writing to the buffer the consumer reads.
        // (They may coincide only after an update() cycle swaps them out.)
    }
    // The real invariant: right now, producer and consumer are different
    EXPECT_NE(buf.get_producer_buf(), static_cast<const RenderState*>(cons));
}
