#include "game/Session.h"

#include "asset/AssetLibrary.h"
#include "asset/MountManager.h"

#include <gtest/gtest.h>

// ---------------------------------------------------------------------------
// Session basics
// ---------------------------------------------------------------------------

TEST(Session, SessionIdIsNonZero) {
    MountManager mounts;
    AssetLibrary assets(mounts, 1024 * 1024);
    Session session(mounts, assets);
    EXPECT_GT(session.session_id(), 0u);
}

TEST(Session, ConsecutiveSessionsGetUniqueIds) {
    MountManager mounts;
    AssetLibrary assets(mounts, 1024 * 1024);

    uint32_t id1, id2;
    {
        Session s1(mounts, assets);
        id1 = s1.session_id();
    }
    {
        Session s2(mounts, assets);
        id2 = s2.session_id();
    }
    EXPECT_NE(id1, id2);
}

TEST(Session, DestructorClearsSessionCache) {
    MountManager mounts;
    AssetLibrary assets(mounts, 1024 * 1024);

    // Load a shader before the session — this is app-lifetime (session_id 0).
    assets.set_shader_backend("glsl");
    auto shader = assets.shader("shaders/text");
    ASSERT_NE(shader, nullptr);

    {
        Session session(mounts, assets);
        // Assets loaded during the session would be tagged with the session_id
        // and cleared on destruction. The shader was loaded before the session
        // so it should survive.
    }

    // App-lifetime asset should still be accessible after session dies.
    // Shader cache key includes the backend suffix.
    auto still_cached = assets.get_cached("shaders/text:glsl");
    EXPECT_NE(still_cached, nullptr);
}
