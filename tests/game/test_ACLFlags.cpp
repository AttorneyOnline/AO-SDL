#include <gtest/gtest.h>

#include "game/ACLFlags.h"

// -- Permission bitflag tests -------------------------------------------------

TEST(ACLFlags, NonePermitsNothing) {
    EXPECT_TRUE(has_permission(ACLPermission::NONE, ACLPermission::NONE));
    EXPECT_FALSE(has_permission(ACLPermission::NONE, ACLPermission::KICK));
    EXPECT_FALSE(has_permission(ACLPermission::NONE, ACLPermission::BAN));
    EXPECT_FALSE(has_permission(ACLPermission::NONE, ACLPermission::SUPER));
}

TEST(ACLFlags, SuperPermitsEverything) {
    EXPECT_TRUE(has_permission(ACLPermission::SUPER, ACLPermission::NONE));
    EXPECT_TRUE(has_permission(ACLPermission::SUPER, ACLPermission::KICK));
    EXPECT_TRUE(has_permission(ACLPermission::SUPER, ACLPermission::BAN));
    EXPECT_TRUE(has_permission(ACLPermission::SUPER, ACLPermission::MODIFY_USERS));
    EXPECT_TRUE(has_permission(ACLPermission::SUPER, ACLPermission::SUPER));
}

TEST(ACLFlags, SinglePermission) {
    auto perms = ACLPermission::KICK;
    EXPECT_TRUE(has_permission(perms, ACLPermission::KICK));
    EXPECT_FALSE(has_permission(perms, ACLPermission::BAN));
    EXPECT_FALSE(has_permission(perms, ACLPermission::MODIFY_USERS));
}

TEST(ACLFlags, CombinedPermissions) {
    auto perms = ACLPermission::KICK | ACLPermission::BAN;
    EXPECT_TRUE(has_permission(perms, ACLPermission::KICK));
    EXPECT_TRUE(has_permission(perms, ACLPermission::BAN));
    EXPECT_FALSE(has_permission(perms, ACLPermission::MODIFY_USERS));
}

TEST(ACLFlags, BitwiseOr) {
    auto a = ACLPermission::KICK;
    auto b = ACLPermission::BAN;
    auto combined = a | b;
    EXPECT_EQ(static_cast<uint32_t>(combined), 3u); // 1 | 2 = 3
}

TEST(ACLFlags, BitwiseAnd) {
    auto all = ACLPermission::KICK | ACLPermission::BAN | ACLPermission::CM;
    auto masked = all & ACLPermission::BAN;
    EXPECT_EQ(static_cast<uint32_t>(masked), static_cast<uint32_t>(ACLPermission::BAN));
}

// -- Role mapping tests -------------------------------------------------------

TEST(ACLFlags, SuperRoleMapping) {
    auto perms = acl_permissions_for_role("SUPER");
    EXPECT_EQ(perms, ACLPermission::SUPER);
}

TEST(ACLFlags, SuperRoleCaseInsensitive) {
    EXPECT_EQ(acl_permissions_for_role("super"), ACLPermission::SUPER);
    EXPECT_EQ(acl_permissions_for_role("Super"), ACLPermission::SUPER);
    EXPECT_EQ(acl_permissions_for_role("SUPER"), ACLPermission::SUPER);
}

TEST(ACLFlags, NoneRoleMapping) {
    EXPECT_EQ(acl_permissions_for_role("NONE"), ACLPermission::NONE);
    EXPECT_EQ(acl_permissions_for_role("none"), ACLPermission::NONE);
    EXPECT_EQ(acl_permissions_for_role(""), ACLPermission::NONE);
}

TEST(ACLFlags, UnknownRoleDefaultsToNone) {
    EXPECT_EQ(acl_permissions_for_role("MODERATOR"), ACLPermission::NONE);
    EXPECT_EQ(acl_permissions_for_role("ADMIN"), ACLPermission::NONE);
    EXPECT_EQ(acl_permissions_for_role("xyz"), ACLPermission::NONE);
}

// -- Permission name tests ----------------------------------------------------

TEST(ACLFlags, PermissionNames) {
    EXPECT_STREQ(permission_name(ACLPermission::NONE), "NONE");
    EXPECT_STREQ(permission_name(ACLPermission::KICK), "KICK");
    EXPECT_STREQ(permission_name(ACLPermission::BAN), "BAN");
    EXPECT_STREQ(permission_name(ACLPermission::MODIFY_USERS), "MODIFY_USERS");
    EXPECT_STREQ(permission_name(ACLPermission::SUPER), "SUPER");
}

// -- AuthType enum test -------------------------------------------------------

TEST(ACLFlags, AuthTypeValues) {
    EXPECT_NE(static_cast<int>(AuthType::SIMPLE), static_cast<int>(AuthType::ADVANCED));
}
