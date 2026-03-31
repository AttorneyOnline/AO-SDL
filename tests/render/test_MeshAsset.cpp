#include "asset/MeshAsset.h"

#include <atomic>
#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <vector>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::vector<MeshVertex> make_quad_verts() {
    return {
        {{0, 0}, {0, 0}},
        {{1, 0}, {1, 0}},
        {{1, 1}, {1, 1}},
        {{0, 1}, {0, 1}},
    };
}

static std::vector<uint32_t> make_quad_indices() {
    return {0, 1, 2, 0, 2, 3};
}

// ---------------------------------------------------------------------------
// Basic MeshAsset behaviour
// ---------------------------------------------------------------------------

TEST(MeshAsset, ConstructionStoresData) {
    auto mesh = std::make_shared<MeshAsset>("test", make_quad_verts(), make_quad_indices());
    EXPECT_EQ(mesh->vertices().size(), 4u);
    EXPECT_EQ(mesh->indices().size(), 6u);
    EXPECT_EQ(mesh->index_count(), 6u);
    EXPECT_EQ(mesh->generation(), 0u);
}

TEST(MeshAsset, UpdateBumpsGeneration) {
    auto mesh = std::make_shared<MeshAsset>("test", make_quad_verts(), make_quad_indices());
    EXPECT_EQ(mesh->generation(), 0u);

    mesh->update(make_quad_verts(), make_quad_indices());
    EXPECT_EQ(mesh->generation(), 1u);
}

TEST(MeshAsset, UpdateToEmptyIndices) {
    auto mesh = std::make_shared<MeshAsset>("test", make_quad_verts(), make_quad_indices());
    EXPECT_EQ(mesh->index_count(), 6u);

    mesh->update(make_quad_verts(), {});
    EXPECT_EQ(mesh->index_count(), 0u);
    EXPECT_TRUE(mesh->indices().empty());
}

// ---------------------------------------------------------------------------
// Reproduces the race that caused the Metal SIGSEGV crash.
//
// The render thread pattern was:
//   1. if (mesh->index_count() > 0)          // reads indices_.size()
//   2.   size = mesh->indices().size()        // reads indices_.size() again
//   3.   newBufferWithBytes(data, size * 4)   // size==0 → nil buffer → crash
//
// Between steps 1 and 2, the game thread could call mesh->update({verts}, {})
// which empties the indices vector. This test hammers that window.
// ---------------------------------------------------------------------------

// With the mutex on MeshAsset, individual accessors are each atomic, so
// two separate calls (index_count() then indices()) can still see different
// generations. The snapshot() API is the correct way to get a consistent view.
TEST(MeshAsset, ConcurrentUpdateIndividualAccessorsAreAtomic) {
    auto mesh = std::make_shared<MeshAsset>("test", make_quad_verts(), make_quad_indices());

    std::atomic<bool> done{false};
    std::atomic<int> reader_iters{0};

    // Writer thread: toggles the mesh between having indices and having none.
    std::thread writer([&] {
        for (int i = 0; i < 100'000 && !done; ++i) {
            if (i % 2 == 0)
                mesh->update(make_quad_verts(), {});
            else
                mesh->update(make_quad_verts(), make_quad_indices());
        }
        done = true;
    });

    // Reader thread: each individual call returns a consistent copy.
    std::thread reader([&] {
        while (!done) {
            size_t count = mesh->index_count();
            // Each call is atomic, so count is always either 0 or 6.
            ASSERT_TRUE(count == 0 || count == 6);
            auto idxs = mesh->indices();
            ASSERT_TRUE(idxs.size() == 0 || idxs.size() == 6);
            reader_iters.fetch_add(1, std::memory_order_relaxed);
        }
    });

    writer.join();
    reader.join();

    EXPECT_GT(reader_iters.load(), 0);
    printf("  [MeshAsset mutex] reader iterations: %d\n", reader_iters.load());
}

// ---------------------------------------------------------------------------
// Validates the fixed pattern: snapshot-then-guard.
//
// The fix in MetalRendererImpl::get_mesh_buffers snapshots indices().size()
// once and checks for zero before creating GPU buffers. This test verifies
// that the snapshot approach never proceeds with zero-length data.
// ---------------------------------------------------------------------------

// The snapshot() API returns vertices, indices, and generation atomically.
// This is the pattern used by the renderer backends to get consistent data.
TEST(MeshAsset, SnapshotIsAtomicallyConsistent) {
    // Start with vertices but no indices. The writer alternates between
    // empty and populated indices, so the reader's generation > 0 check
    // is only valid when indices came from an update (not the constructor).
    auto mesh = std::make_shared<MeshAsset>("test", make_quad_verts(), std::vector<uint32_t>{});

    std::atomic<bool> done{false};
    std::atomic<int> reader_iters{0};
    std::atomic<int> safely_skipped{0};

    // Both threads sleep on coprime intervals (7 and 11) to create varied
    // interleaving without explicit coordination. The writer runs enough
    // iterations with long enough sleeps that the reader is guaranteed to
    // be scheduled and make progress before the writer finishes.
    using ms = std::chrono::milliseconds;

    std::thread writer([&] {
        for (int i = 0; i < 1'000 && !done; ++i) {
            if (i % 2 == 0)
                mesh->update(make_quad_verts(), {});
            else
                mesh->update(make_quad_verts(), make_quad_indices());
            if (i % 7 == 0)
                std::this_thread::sleep_for(ms(1));
        }
        done = true;
    });

    std::thread reader([&] {
        // do-while guarantees at least one snapshot even if the reader
        // thread starts after the writer finishes. In the common case
        // both threads overlap and stress-test the mutex.
        do {
            auto snap = mesh->snapshot();

            // Snapshot must be internally consistent: if indices are present,
            // vertices must also be present (and vice versa for our test data).
            // Use EXPECT (not ASSERT) — ASSERT kills the thread on failure,
            // swallowing the real error and leaving reader_iters at 0.
            if (!snap.indices.empty()) {
                EXPECT_EQ(snap.indices.size(), 6u);
                EXPECT_EQ(snap.vertices.size(), 4u);
                EXPECT_GT(snap.generation, 0u);
            }
            else {
                EXPECT_EQ(snap.vertices.size(), 4u);
                safely_skipped.fetch_add(1, std::memory_order_relaxed);
            }
            reader_iters.fetch_add(1, std::memory_order_relaxed);
            if (reader_iters.load(std::memory_order_relaxed) % 11 == 0)
                std::this_thread::sleep_for(ms(1));
        } while (!done);
    });

    writer.join();
    reader.join();

    EXPECT_GT(reader_iters.load(), 0);
    printf("  [MeshAsset snapshot] reader iterations: %d, empty-indices reads: %d\n", reader_iters.load(),
           safely_skipped.load());
}
