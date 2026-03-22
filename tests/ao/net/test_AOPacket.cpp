#include "net/ao/AOPacket.h"
// Include PacketTypes so all registrars run (needed for deserialization to
// produce the correct derived type rather than a bare AOPacket).
#include "net/ao/PacketTypes.h"

#include <gtest/gtest.h>
#include <stdexcept>
#include <string>

// ---------------------------------------------------------------------------
// serialize
// ---------------------------------------------------------------------------

TEST(AOPacket, SerializeHeaderOnly) {
    AOPacket pkt("RC", {});
    EXPECT_EQ(pkt.serialize(), "RC#%");
}

TEST(AOPacket, SerializeSingleField) {
    AOPacket pkt("HI", {"myhwid"});
    EXPECT_EQ(pkt.serialize(), "HI#myhwid#%");
}

TEST(AOPacket, SerializeMultipleFields) {
    AOPacket pkt("ID", {"42", "software", "1.0"});
    EXPECT_EQ(pkt.serialize(), "ID#42#software#1.0#%");
}

// ---------------------------------------------------------------------------
// deserialize
// ---------------------------------------------------------------------------

TEST(AOPacket, DeserializeMissingDelimiterThrows) {
    EXPECT_THROW(AOPacket::deserialize("RC"), std::invalid_argument);
}

TEST(AOPacket, DeserializeEmptyStringThrows) {
    EXPECT_THROW(AOPacket::deserialize(""), std::invalid_argument);
}

TEST(AOPacket, DeserializeHeaderOnlyPacket) {
    auto pkt = AOPacket::deserialize("RC#%");
    ASSERT_NE(pkt, nullptr);
    EXPECT_TRUE(pkt->is_valid());
    EXPECT_EQ(pkt->serialize(), "RC#%");
}

TEST(AOPacket, DeserializeSingleField) {
    auto pkt = AOPacket::deserialize("HI#myhwid#%");
    ASSERT_NE(pkt, nullptr);
    EXPECT_EQ(pkt->serialize(), "HI#myhwid#%");
}

TEST(AOPacket, DeserializeMultipleFields) {
    auto pkt = AOPacket::deserialize("ID#42#software#1.0#%");
    ASSERT_NE(pkt, nullptr);
    EXPECT_EQ(pkt->serialize(), "ID#42#software#1.0#%");
}

TEST(AOPacket, DeserializeUnknownHeaderProducesBasePacket) {
    // Unknown headers fall back to a bare AOPacket; serialization round-trips.
    auto pkt = AOPacket::deserialize("UNKNOWN#field1#field2#%");
    ASSERT_NE(pkt, nullptr);
    EXPECT_TRUE(pkt->is_valid());
    EXPECT_EQ(pkt->serialize(), "UNKNOWN#field1#field2#%");
}

// ---------------------------------------------------------------------------
// Serialize → deserialize round-trip
// ---------------------------------------------------------------------------

TEST(AOPacket, SerializeDeserializeRoundTrip) {
    AOPacket original("CT", {"Phoenix", "Objection!", "0"});
    std::string wire = original.serialize();
    auto recovered = AOPacket::deserialize(wire);
    ASSERT_NE(recovered, nullptr);
    EXPECT_EQ(recovered->serialize(), wire);
}

TEST(AOPacket, IsValidFalseForDefaultConstruct) {
    AOPacket pkt;
    EXPECT_FALSE(pkt.is_valid());
}
