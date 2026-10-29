// Copyright 2026 Citra Emulator Project / Azahar Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the misc/licenses/gplv2.txt file included.

#include <algorithm>
#include <array>
#include <chrono>
#include <mutex>
#include <set>
#include <thread>
#include <catch2/catch_test_macros.hpp>
#include "common/scope_exit.h"
#include "network/network.h"

namespace {

template <typename Predicate>
bool WaitFor(Predicate predicate) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(15);
    while (!predicate()) {
        if (std::chrono::steady_clock::now() >= deadline) {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return true;
}

} // namespace

TEST_CASE("Room delivers eight members' broadcast and unicast bursts", "[network][multiplayer]") {
    constexpr std::size_t Players = 8;
    constexpr u8 Frames = 60;
    constexpr std::array<std::size_t, 7> PayloadSizes{9, 10, 11, 73, 535, 607, 0x5C6};
    REQUIRE(Network::Init());
    SCOPE_EXIT({ Network::Shutdown(); });
    auto room = Network::GetRoom().lock();
    REQUIRE(room);
    u16 port = 39470;
    for (; port < 39490; ++port) {
        if (room->Create("Eight player regression", "", "127.0.0.1", port, "", Players, "", "", 0,
                         std::make_unique<Network::VerifyUser::NullBackend>())) {
            break;
        }
    }
    REQUIRE(port < 39490);

    std::mutex mutex;
    std::array<std::set<std::array<u8, 3>>, Players> received;
    bool invalid_packet = false;
    std::array<std::unique_ptr<Network::RoomMember>, Players> members;
    SCOPE_EXIT({
        for (auto& member : members) {
            if (member && member->IsConnected()) {
                member->Leave();
            }
        }
    });
    for (std::size_t i = 0; i < Players; ++i) {
        members[i] = std::make_unique<Network::RoomMember>();
        members[i]->BindOnWifiPacketReceived([&, i](const Network::WifiPacket& packet) {
            std::scoped_lock lock(mutex);
            if (packet.data.size() < 3) {
                invalid_packet = true;
                return;
            }
            const auto sender = packet.data[0];
            const auto recipient = packet.data[1];
            const auto frame = packet.data[2];
            const auto length = PayloadSizes[frame % PayloadSizes.size()];
            if (sender >= Players || sender == i || frame >= Frames ||
                (recipient != 0xFF && recipient != i) || packet.data.size() != length ||
                !std::all_of(packet.data.begin() + 3, packet.data.end(),
                             [](u8 byte) { return byte == 0xA5; })) {
                invalid_packet = true;
                return;
            }
            if (!received[i].insert({sender, recipient, frame}).second) {
                invalid_packet = true;
            }
        });
        members[i]->Join("Player" + std::to_string(i), "Console" + std::to_string(i), "127.0.0.1",
                         port);
    }
    REQUIRE(WaitFor([&] {
        return std::all_of(members.begin(), members.end(), [](const auto& member) {
            return member->GetState() == Network::RoomMember::State::Joined;
        });
    }));

    for (u8 frame = 0; frame < Frames; ++frame) {
        for (u8 sender = 0; sender < Players; ++sender) {
            Network::WifiPacket packet{};
            packet.type = Network::WifiPacket::PacketType::Data;
            packet.channel = 11;
            packet.transmitter_address = members[sender]->GetMacAddress();
            packet.data.resize(PayloadSizes[frame % PayloadSizes.size()], 0xA5);
            packet.data[0] = sender;
            packet.data[2] = frame;
            for (u8 recipient = 0; recipient <= Players; ++recipient) {
                if (recipient == sender) {
                    continue;
                }
                packet.data[1] = recipient == Players ? 0xFF : recipient;
                packet.destination_address = recipient == Players
                                                 ? Network::BroadcastMac
                                                 : members[recipient]->GetMacAddress();
                members[sender]->SendWifiPacket(packet);
            }
        }
    }
    REQUIRE(WaitFor([&] {
        std::scoped_lock lock(mutex);
        return std::all_of(received.begin(), received.end(), [](const auto& packets) {
            return packets.size() == 2 * (Players - 1) * Frames;
        });
    }));
    {
        std::scoped_lock lock(mutex);
        CHECK_FALSE(invalid_packet);
    }
}
