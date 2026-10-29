// Copyright 2026 Citra Emulator Project / Azahar Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the misc/licenses/gplv2.txt file included.

#include <catch2/catch_test_macros.hpp>
#include "core/core.h"
#include "core/core_timing.h"
#include "core/hle/ipc.h"
#include "core/hle/kernel/hle_ipc.h"
#include "core/hle/kernel/server_session.h"
#include "core/hle/service/nwm/nwm_uds.h"
#include "core/hle/service/nwm/uds_data.h"
#include "core/memory.h"

namespace Service::NWM {

TEST_CASE("UDS concurrent joins only accept their own assigned node ID", "[core][nwm]") {
    NodeList nodes(8);
    for (u16 i = 0; i < nodes.size(); ++i) {
        nodes[i].network_node_id = i + 1;
    }
    for (u16 joining = 2; joining <= 8; ++joining) {
        const MacAddress recipient{0, 0x1F, 0x32, 0, 0, static_cast<u8>(joining)};
        const auto frame = GenerateEAPoLLogoffFrame(recipient, joining, nodes, 8, joining);
        const auto packet = ParseEAPoLLogoffFrame(frame);
        CHECK(packet.client_mac_address == recipient);
        CHECK(packet.assigned_node_id == joining);
        for (u16 other = 2; other <= 8; ++other) {
            const MacAddress local{0, 0x1F, 0x32, 0, 0, static_cast<u8>(other)};
            CHECK(IsEAPoLLogoffForClient(packet, local, ConnectionType::Client, other) ==
                  (joining == other));
        }
        auto legacy_packet = packet;
        legacy_packet.client_mac_address = {};
        CHECK(IsEAPoLLogoffForClient(legacy_packet, recipient, ConnectionType::Client, joining));
        CHECK_FALSE(IsEAPoLLogoffForClient(legacy_packet, recipient, ConnectionType::Client, 0));
        CHECK_FALSE(IsEAPoLLogoffForClient(legacy_packet, recipient, ConnectionType::Client,
                                           joining == 8 ? 2 : joining + 1));
        CHECK_FALSE(IsEAPoLLogoffForClient(legacy_packet, recipient, ConnectionType::Spectator, 0));
    }
}

TEST_CASE("UDS rejects stale responses from a different connection type", "[core][nwm]") {
    const MacAddress local{0, 0x1F, 0x32, 0, 0, 2};
    EAPoLLogoffPacket packet{};
    packet.max_nodes = 8;
    for (const auto recipient : {local, MacAddress{}}) {
        packet.client_mac_address = recipient;
        packet.assigned_node_id = NodeIDSpec;
        CHECK(IsEAPoLLogoffForClient(packet, local, ConnectionType::Spectator, 0));
        CHECK_FALSE(IsEAPoLLogoffForClient(packet, local, ConnectionType::Client, 2));
        for (const u16 invalid_node : {1, 9, 0xFFFF}) {
            packet.assigned_node_id = invalid_node;
            CHECK_FALSE(
                IsEAPoLLogoffForClient(packet, local, ConnectionType::Client, invalid_node));
        }
        packet.assigned_node_id = 2;
        CHECK(IsEAPoLLogoffForClient(packet, local, ConnectionType::Client, 2));
        CHECK_FALSE(IsEAPoLLogoffForClient(packet, local, ConnectionType::Spectator, 0));
    }
}

TEST_CASE("UDS PullPacket preserves every successful payload length", "[core][nwm]") {
    Core::System system;
    Core::Timing timing(1, 100);
    Memory::MemorySystem memory{system};
    Kernel::KernelSystem kernel(memory, timing, [] {}, Kernel::MemoryMode::NewProd, 1);
    auto [server, client] = kernel.CreateSessionPair();

    // In particular, lengths 9, 10 and 11 overlap ResultStatus receive error values.
    for (int length = 0; length <= 0x5C6; ++length) {
        CAPTURE(length);
        Kernel::HLERequestContext context(kernel, server, nullptr);
        context.CommandBuffer()[0] = IPC::MakeHeader(0x14, 3, 0);
        SecureDataHeader header{};
        header.src_node_id = length ? 8 : 0;
        std::vector<u8> payload(length, 0xA5);
        WritePullPacketResponse(context, length, header, payload);

        const auto* reply = context.CommandBuffer();
        REQUIRE(reply[0] == IPC::MakeHeader(0x14, 3, 2));
        REQUIRE(reply[1] == ResultSuccess.raw);
        CHECK(reply[2] == static_cast<u32>(length));
        CHECK(reply[3] == (length ? 8 : 0));
        CHECK(context.GetStaticBuffer(0) == payload);
    }
}

TEST_CASE("UDS PullPacket preserves receive errors", "[core][nwm]") {
    Core::System system;
    Core::Timing timing(1, 100);
    Memory::MemorySystem memory{system};
    Kernel::KernelSystem kernel(memory, timing, [] {}, Kernel::MemoryMode::NewProd, 1);
    auto [server, client] = kernel.CreateSessionPair();

    for (const auto error : {ResultStatus::RecvError_NotConnected, ResultStatus::RecvError_BadNode,
                             ResultStatus::RecvError_PacketSizeTooLarge}) {
        CAPTURE(static_cast<int>(error));
        Kernel::HLERequestContext context(kernel, server, nullptr);
        context.CommandBuffer()[0] = IPC::MakeHeader(0x14, 3, 0);
        WritePullPacketResponse(context, Common::Unexpected(error), {}, {});

        const Result expected = error == ResultStatus::RecvError_PacketSizeTooLarge
                                    ? Result(ErrorDescription::TooLarge, ErrorModule::UDS,
                                             ErrorSummary::WrongArgument, ErrorLevel::Usage)
                                    : Result(ErrorDescription::NotAuthorized, ErrorModule::UDS,
                                             ErrorSummary::InvalidState, ErrorLevel::Status);
        CHECK(context.CommandBuffer()[0] == IPC::MakeHeader(0x14, 1, 0));
        CHECK(context.CommandBuffer()[1] == expected.raw);
    }
}

} // namespace Service::NWM
