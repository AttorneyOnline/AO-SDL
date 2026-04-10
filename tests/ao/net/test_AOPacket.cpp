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

// ---------------------------------------------------------------------------
// ao_packet::safe_dispatch — crash-safety boundary
// ---------------------------------------------------------------------------
//
// Regression coverage for the "webAO MS#...#NaN#..." crash: before this
// change, an un-parseable numeric field inside an MS packet constructor
// threw std::invalid_argument, which escaped AOPacket::deserialize, escaped
// AOServer::on_client_message (deserialize was called outside the try/catch),
// escaped the worker thread lambda, and terminated the entire kagami process
// with std::terminate. safe_dispatch wraps *both* parse and dispatch in a
// single try/catch so no exception can leak out to the worker thread.

TEST(AOPacketSafeDispatch, ValidPacketIsDispatchedOnce) {
    int calls = 0;
    auto result = ao_packet::safe_dispatch("HI#myhwid#%", "unit-test", [&](AOPacket& pkt) {
        ++calls;
        EXPECT_EQ(pkt.get_header(), "HI");
    });
    EXPECT_EQ(result, ao_packet::DispatchResult::Ok);
    EXPECT_EQ(calls, 1);
}

TEST(AOPacketSafeDispatch, ParseErrorIsSwallowed) {
    int calls = 0;
    // Missing #% delimiter — AOPacket::deserialize throws std::invalid_argument.
    auto result = ao_packet::safe_dispatch("RC", "unit-test", [&](AOPacket&) { ++calls; });
    EXPECT_EQ(result, ao_packet::DispatchResult::ParseFailed);
    EXPECT_EQ(calls, 0);
}

TEST(AOPacketSafeDispatch, HandlerExceptionIsSwallowed) {
    auto result = ao_packet::safe_dispatch("HI#myhwid#%", "unit-test",
                                           [](AOPacket&) { throw std::runtime_error("handler blew up"); });
    EXPECT_EQ(result, ao_packet::DispatchResult::HandlerFailed);
}

TEST(AOPacketSafeDispatch, WebaoMsNanDoesNotCrash) {
    // Real payload captured from webAO parsing neuro/char.ini — field 9
    // (sfx_delay) is "NaN" because the INI's [SoundT] block is malformed.
    // Before the fix, AOPacketMS's std::stoi threw std::invalid_argument,
    // which escaped deserialize and killed the server process.
    // After the fix, safe_stoi inside the MS constructor recovers with 0,
    // so the packet is dispatched normally. safe_dispatch would also catch
    // any exception that did escape — belt-and-suspenders.
    constexpr const char* webao_wire = "MS#1#-#neuro#/normal#test#def#02  = 03  = 04  = 05  = 06  = 07  = 08  = "
                                       "0#0#40#NaN#0#0#0#0#0##-1#0#0#0#0#-#-#-#0#||#%";

    bool dispatched = false;
    auto result = ao_packet::safe_dispatch(webao_wire, "unit-test", [&](AOPacket& pkt) {
        dispatched = true;
        EXPECT_EQ(pkt.get_header(), "MS");
    });

    EXPECT_EQ(result, ao_packet::DispatchResult::Ok);
    EXPECT_TRUE(dispatched);
}
