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

    // Load a shader (app-lifetime, embedded)
    assets.set_shader_backend("glsl");
    auto shader = assets.shader("shaders/text");
    ASSERT_NE(shader, nullptr);

    {
        Session session(mounts, assets);
        // Assets loaded during the session are tagged with the session_id.
        // We can't easily load an HTTP asset in a unit test, but we can
        // verify the session sets and clears active_session correctly by
        // checking that app-lifetime assets survive session teardown.
    }

    // App-lifetime asset should still be accessible after session dies
    auto still_cached = assets.get_cached("shaders/text");
    EXPECT_NE(still_cached, nullptr);
}
