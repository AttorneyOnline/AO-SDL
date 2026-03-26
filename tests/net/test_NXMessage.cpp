#include <gtest/gtest.h>

#include "net/nx/NXMessage.h"

TEST(NXMessageTest, DeserializeValidMessage) {
    auto msg = NXMessage::deserialize(R"({"type":"ic_message","schema_version":1,"text":"hello"})");
    ASSERT_TRUE(msg.has_value());
    EXPECT_EQ(msg->type(), "ic_message");
    EXPECT_EQ(msg->schema_version(), 1);
    EXPECT_EQ(msg->field<std::string>("text"), "hello");
}

TEST(NXMessageTest, DeserializeRejectsNonObject) {
    EXPECT_FALSE(NXMessage::deserialize("[]").has_value());
    EXPECT_FALSE(NXMessage::deserialize("42").has_value());
    EXPECT_FALSE(NXMessage::deserialize("null").has_value());
}

TEST(NXMessageTest, DeserializeRejectsInvalidJson) {
    EXPECT_FALSE(NXMessage::deserialize("{not json}").has_value());
    EXPECT_FALSE(NXMessage::deserialize("").has_value());
}

TEST(NXMessageTest, DeserializeRejectsMissingType) {
    EXPECT_FALSE(NXMessage::deserialize(R"({"schema_version":1})").has_value());
}

TEST(NXMessageTest, DeserializeRejectsMissingSchemaVersion) {
    EXPECT_FALSE(NXMessage::deserialize(R"({"type":"test"})").has_value());
}

TEST(NXMessageTest, SerializeRoundTrip) {
    auto msg = NXMessage::deserialize(R"({"type":"music","schema_version":2,"track":"objection.opus"})");
    ASSERT_TRUE(msg.has_value());
    auto serialized = msg->serialize();
    auto msg2 = NXMessage::deserialize(serialized);
    ASSERT_TRUE(msg2.has_value());
    EXPECT_EQ(msg2->type(), "music");
    EXPECT_EQ(msg2->schema_version(), 2);
    EXPECT_EQ(msg2->field<std::string>("track"), "objection.opus");
}

TEST(NXMessageTest, FieldWithDefault) {
    auto msg = NXMessage::deserialize(R"({"type":"test","schema_version":1})");
    ASSERT_TRUE(msg.has_value());
    EXPECT_EQ(msg->field<std::string>("missing", "fallback"), "fallback");
    EXPECT_EQ(msg->field<int>("missing", 42), 42);
}

TEST(NXMessageTest, FieldTypeMismatchReturnsFallback) {
    auto msg = NXMessage::deserialize(R"({"type":"test","schema_version":1,"val":"not_int"})");
    ASSERT_TRUE(msg.has_value());
    EXPECT_EQ(msg->field<int>("val", -1), -1);
}
