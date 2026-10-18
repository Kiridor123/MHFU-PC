// The built-in ad hoc server, checked over the loopback address against the
// game's own client and against a second player that speaks the protocols
// byte by byte, as another emulator would; then local network discovery.
// Needs no game data. Every wait is bounded.
#include "adhoc/client.hpp"
#include "adhoc/discovery.hpp"
#include "adhoc/protocol.hpp"
#include "adhoc/server.hpp"
#include "adhoc/sockets.hpp"

#include <chrono>
#include <cstdlib>
#include <cstdio>
#include <functional>
#include <iostream>
#include <optional>
#include <string>
#include <thread>

namespace {

using namespace mhp3rd::adhoc;
using namespace mhp3rd::adhoc::net;
using Clock = std::chrono::steady_clock;

int failures = 0;

void check(bool condition, const std::string &what) {
    std::cout << (condition ? "ok   " : "FAIL ") << what << std::endl;
    if (!condition) ++failures;
}

bool wait_until(const std::function<bool()> &done, int timeout_ms = 5000) {
    const auto until = Clock::now() + std::chrono::milliseconds(timeout_ms);
    while (Clock::now() < until) {
        if (done()) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return done();
}

// One TCP connection driven by hand.
struct Raw {
    Socket s{kNoSocket};
    std::string pending;

    explicit Raw(std::uint16_t port) {
        s = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        const Address address = Address::ipv4_address(htonl(INADDR_LOOPBACK), port);
        if (::connect(s, reinterpret_cast<const sockaddr *>(&address.storage), address.length) != 0) {
            close_socket(s);
            s = kNoSocket;
        }
        if (s != kNoSocket) set_nonblocking(s);
        if (s != kNoSocket) no_sigpipe(s);
    }
    ~Raw() { close(); }
    void close() {
        if (s != kNoSocket) close_socket(s);
        s = kNoSocket;
    }
    void send(const std::string &data) { ::send(s, data.data(), static_cast<int>(data.size()), kSendFlags); }
    // Reads until `size` bytes are there, or gives up.
    std::optional<std::string> read(std::size_t size, int timeout_ms = 3000) {
        const auto until = Clock::now() + std::chrono::milliseconds(timeout_ms);
        while (pending.size() < size && Clock::now() < until) {
            char buffer[4096];
            const auto count = ::recv(s, buffer, static_cast<int>(sizeof(buffer)), 0);
            if (count > 0)
                pending.append(buffer, static_cast<std::size_t>(count));
            else if (count == 0)
                break;
            else
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        if (pending.size() < size) return std::nullopt;
        std::string out = pending.substr(0, size);
        pending.erase(0, size);
        return out;
    }
    // True once the server has closed the connection.
    bool closed(int timeout_ms = 3000) {
        const auto until = Clock::now() + std::chrono::milliseconds(timeout_ms);
        while (Clock::now() < until) {
            char buffer[256];
            const auto count = ::recv(s, buffer, static_cast<int>(sizeof(buffer)), 0);
            if (count == 0) return true;
            if (count > 0)
                pending.append(buffer, static_cast<std::size_t>(count));
            else if (!would_block(socket_error()))
                return true;
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        return false;
    }
};

const Mac kMacA{0x02, 0x11, 0x22, 0x33, 0x44, 0x0A};
const Mac kMacB{0x02, 0x11, 0x22, 0x33, 0x44, 0x0B};
constexpr const char *kProduct = "ULJM05800";
constexpr const char *kGroup = "MHP3Q000";

std::uint16_t start_server(Server &server) {
    for (std::uint16_t port = 37312; port < 37400; port += 2) {
        ServerConfig config;
        config.adhocctl_port = port;
        if (server.start(config)) return port;
    }
    return 0;
}

void server_and_client() {
    Server server;
    const std::uint16_t port = start_server(server);
    check(port != 0u, "the server starts on a free port pair");
    if (port == 0u) return;
    const std::uint16_t relay_port = relay_port_for(port);

    Server second;
    ServerConfig same;
    same.adhocctl_port = port;
    check(!second.start(same) && !second.status().error.empty(), "a second server on the same port is refused");

    // Player A: the game's client.
    Client &a = Client::get();
    Identity identity;
    identity.server = "127.0.0.1:" + std::to_string(port);
    identity.nickname = "HunterA";
    identity.mac = kMacA;
    identity.product = kProduct;
    a.start(identity);
    check(wait_until([&] { return a.server_state() == ServerState::Online; }), "A logs in");
    a.join(kGroup);
    check(wait_until([&] { return a.in_group(); }), "A creates and joins the group");
    check(a.group() && a.group()->host == kMacA, "A is the group's host");

    // Player B: raw protocol.
    Raw b(port);
    check(b.s != kNoSocket, "B connects to adhocctl");
    b.send(ctl::login(kMacB, "HunterB", kProduct));
    b.send(ctl::connect(kGroup));
    const auto notice = b.read(ctl::server_packet_size(ctl::kConnect));
    check(notice && static_cast<std::uint8_t>((*notice)[0]) == ctl::kConnect &&
            wire::get_fixed(notice->data() + 1, ctl::kNicknameLength) == "HunterA" &&
            wire::get_mac(notice->data() + 1 + ctl::kNicknameLength) == kMacA,
        "B is told A is in the group");
    const auto bssid = b.read(ctl::server_packet_size(ctl::kConnectBssid));
    check(bssid && static_cast<std::uint8_t>((*bssid)[0]) == ctl::kConnectBssid &&
            wire::get_mac(bssid->data() + 1) == kMacA,
        "B's join is confirmed with A as the host");
    check(wait_until([&] { return a.peers().size() == 1u && a.peers()[0].mac == kMacB; }), "A sees B join");

    b.send(ctl::opcode_only(ctl::kScan));
    const auto scan = b.read(ctl::server_packet_size(ctl::kScan));
    const auto complete = b.read(1u);
    check(scan && wire::get_fixed(scan->data() + 1, ctl::kGroupNameLength) == kGroup &&
            wire::get_mac(scan->data() + 1 + ctl::kGroupNameLength) == kMacA && complete &&
            static_cast<std::uint8_t>((*complete)[0]) == ctl::kScanComplete,
        "a scan lists the group and completes");

    // Datagrams.
    const int pdp = a.pdp_open(10000, 8192);
    Raw b_pdp(relay_port);
    b_pdp.send(relay::init(relay::kInitPdp, kMacB, 10000, Mac{}, 0));
    check(wait_until([&] {
        const Diagnostics d = a.diagnostics();
        return d.relay_links_wanted == 1u && d.relay_links_up == 1u;
    }),
        "A's datagram socket links to the relay");
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // the server reads B's init
    const std::string hello = "hello from A";
    a.pdp_send(pdp, kBroadcastMac, 10000, hello.data(), hello.size());
    const auto header = b_pdp.read(relay::kPdpHeaderSize);
    const auto body = header ? b_pdp.read(wire::get32(header->data() + 10)) : std::nullopt;
    check(header && wire::get_mac(header->data()) == kMacA && wire::get16(header->data() + 8) == 10000 && body &&
            *body == hello,
        "A's broadcast datagram reaches B with A as the sender");
    const std::string reply = "hello from B";
    b_pdp.send(relay::pdp_header(kMacA, 10000, static_cast<std::uint32_t>(reply.size())) + reply);
    std::optional<Datagram> got;
    check(wait_until([&] { return (got = a.pdp_receive(pdp)).has_value(); }) && got->source == kMacB &&
            got->port == 10000 && got->data == reply,
        "B's datagram reaches A");
    b_pdp.send(relay::pdp_header(Mac{0x02, 9, 9, 9, 9, 9}, 10000, 3) + "xyz");

    // Direct UDP datagrams (PPSSPP compatibility with portOffset=10000).
    Socket b_udp = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    set_nonblocking(b_udp);
    sockaddr_in b_addr{};
    b_addr.sin_family = AF_INET;
    b_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    b_addr.sin_port = htons(20002);
    ::bind(b_udp, reinterpret_cast<const sockaddr *>(&b_addr), sizeof(b_addr));

    // B sends a UDP datagram to A's PDP port 10000 (shifted by portOffset=10000 -> 20000).
    const std::string udp_msg = "udp hello from PPSSPP peer B";
    sockaddr_in a_udp_addr{};
    a_udp_addr.sin_family = AF_INET;
    a_udp_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    a_udp_addr.sin_port = htons(20000);
    ::sendto(b_udp, udp_msg.data(), static_cast<int>(udp_msg.size()), 0,
        reinterpret_cast<const sockaddr *>(&a_udp_addr), sizeof(a_udp_addr));

    std::optional<Datagram> udp_got;
    check(wait_until([&] { return (udp_got = a.pdp_receive(pdp)).has_value(); }) && udp_got->source == kMacB &&
            udp_got->port == 10000 && udp_got->data == udp_msg,
        "A receives a direct UDP datagram from peer B");

    // A sends a datagram back to B (which sends via UDP to B's learned port 20002).
    const std::string a_to_b_udp = "reply from A to B via UDP";
    a.pdp_send(pdp, kMacB, 10000, a_to_b_udp.data(), a_to_b_udp.size());
    char udp_buf[512] = {};
    const auto recv_b = wait_until([&] {
        const auto count = ::recv(b_udp, udp_buf, sizeof(udp_buf), 0);
        return count > 0 && std::string(udp_buf, count) == a_to_b_udp;
    });
    check(recv_b, "B receives A's datagram directly over UDP");

    // Deduplication test: send same packet over both relay and UDP rapidly.
    const std::string dup_msg = "test duplicate suppression";
    b_pdp.send(relay::pdp_header(kMacA, 10000, static_cast<std::uint32_t>(dup_msg.size())) + dup_msg);
    ::sendto(b_udp, dup_msg.data(), static_cast<int>(dup_msg.size()), 0,
        reinterpret_cast<const sockaddr *>(&a_udp_addr), sizeof(a_udp_addr));
    std::optional<Datagram> dup_first;
    check(wait_until([&] { return (dup_first = a.pdp_receive(pdp)).has_value(); }) && dup_first->data == dup_msg,
        "first instance of duplicated packet is received");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::optional<Datagram> dup_second = a.pdp_receive(pdp);
    check(!dup_second.has_value(), "duplicate packet within suppression window was dropped");
    close_socket(b_udp);

    // In-game adhocctl chat (PPSSPP cross-play compatibility).
    const std::string ppsspp_chat = "Good hunting!";
    b.send(ctl::chat(ppsspp_chat));
    std::optional<ChatMessage> chat_from_b;
    check(wait_until([&] { return (chat_from_b = a.take_latest_chat()).has_value(); }) &&
            chat_from_b->sender == "HunterB" && chat_from_b->message == ppsspp_chat,
        "A receives B's chat message with nickname");

    const std::string pokke_chat = "Thanks, let's go!";
    a.send_chat(pokke_chat);
    const auto chat_packet = b.read(ctl::server_packet_size(ctl::kChat));
    check(chat_packet && static_cast<std::uint8_t>((*chat_packet)[0]) == ctl::kChat &&
            wire::get_fixed(chat_packet->data() + 1, ctl::kChatLength) == pokke_chat &&
            wire::get_fixed(chat_packet->data() + 1 + ctl::kChatLength, ctl::kNicknameLength) == "HunterA",
        "B receives A's chat message over adhocctl");

    // A stream from B to A's listening socket.
    const int listener = a.ptp_listen(20001, 8192, 1);
    check(wait_until([&] {
        const Diagnostics d = a.diagnostics();
        return d.relay_links_up == 2u;
    }),
        "A's listening socket links to the relay");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    Raw b_stream(relay_port);
    b_stream.send(relay::init(relay::kInitPtpConnect, kMacB, 30000, kMacA, 20001));
    int accepted = 0;
    check(wait_until([&] { return (accepted = a.ptp_accept(listener)) != 0; }), "A gets B's connection request");
    const auto established = b_stream.read(relay::kPtpNoticeSize);
    check(established && wire::get_mac(established->data()) == kMacA && wire::get16(established->data() + 8) == 20001,
        "B is told the stream is established");
    const std::string ping = "quest start";
    check(a.ptp_send(accepted, ping.data(), ping.size()) == ping.size(), "A queues stream data");
    const auto block = b_stream.read(relay::kPtpHeaderSize + ping.size());
    check(block && wire::get32(block->data()) == ping.size() && block->substr(4) == ping, "B receives it");
    std::string pong = "ready";
    std::string framed;
    wire::put32(framed, static_cast<std::uint32_t>(pong.size()));
    b_stream.send(framed + pong);
    char buffer[64] = {};
    std::size_t received = 0;
    check(wait_until([&] { return (received = a.ptp_receive(accepted, buffer, sizeof(buffer))) != 0u; }) &&
            std::string(buffer, received) == pong,
        "A receives B's stream data");
    b_stream.close();
    check(wait_until([&] { return a.ptp_info(accepted).state == StreamState::Disconnected; }),
        "B closing the stream reaches A as a disconnect");
    a.ptp_close(accepted);

    // Direct TCP stream from B to A (PPSSPP compatibility on port 20001 + 10000 = 30001).
    Socket direct_b = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    set_nonblocking(direct_b);
    sockaddr_in direct_dest{};
    direct_dest.sin_family = AF_INET;
    direct_dest.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    direct_dest.sin_port = htons(30001);
    ::connect(direct_b, reinterpret_cast<const sockaddr *>(&direct_dest), sizeof(direct_dest));
    int direct_accepted = 0;
    check(wait_until([&] { return (direct_accepted = a.ptp_accept(listener)) != 0; }),
        "A accepts direct TCP connection from B");
    const std::string direct_ping = "direct quest sync";
    check(a.ptp_send(direct_accepted, direct_ping.data(), direct_ping.size()) == direct_ping.size(),
        "A sends data over direct TCP");
    char direct_buf[64] = {};
    const auto recv_direct = wait_until([&] {
        const auto count = ::recv(direct_b, direct_buf, sizeof(direct_buf), 0);
        return count > 0 && std::string(direct_buf, count) == direct_ping;
    });
    check(recv_direct, "B receives A's direct TCP stream data");
    const std::string direct_pong = "direct sync ok";
    ::send(direct_b, direct_pong.data(), static_cast<int>(direct_pong.size()), 0);
    char direct_rx[64] = {};
    std::size_t direct_rcvd = 0;
    check(wait_until([&] { return (direct_rcvd = a.ptp_receive(direct_accepted, direct_rx, sizeof(direct_rx))) != 0u; }) &&
            std::string(direct_rx, direct_rcvd) == direct_pong,
        "A receives B's direct TCP stream data");
    close_socket(direct_b);
    check(wait_until([&] { return a.ptp_info(direct_accepted).state == StreamState::Disconnected; }),
        "B closing direct TCP stream reaches A as disconnect");
    a.ptp_close(direct_accepted);

    // A stream from A to B's listening socket.
    Raw b_listen(relay_port);
    b_listen.send(relay::init(relay::kInitPtpListen, kMacB, 20002, Mac{}, 0));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    const int opened = a.ptp_open(0, kMacB, 20002, 8192, 200000, 10);
    const auto request = b_listen.read(relay::kPtpNoticeSize);
    check(request && wire::get_mac(request->data()) == kMacA, "B's listening socket hears A's request");
    if (request) {
        const std::uint16_t a_port = wire::get16(request->data() + 8);
        Raw b_accept(relay_port);
        b_accept.send(relay::init(relay::kInitPtpAccept, kMacB, 20002, kMacA, a_port));
        const auto accept_notice = b_accept.read(relay::kPtpNoticeSize);
        check(accept_notice && wire::get_mac(accept_notice->data()) == kMacA &&
                wire::get16(accept_notice->data() + 8) == a_port,
            "B's accepting connection is paired");
        check(wait_until([&] { return a.ptp_info(opened).state == StreamState::Established; }),
            "A's stream is established");
    }
    Raw stray(relay_port);
    stray.send(relay::init(relay::kInitPtpAccept, kMacB, 20003, kMacA, 1234));
    check(stray.closed(), "an accept nobody waits for is refused");

    // Leaving, and logging in again with the same MAC.
    b.send(ctl::opcode_only(ctl::kDisconnect));
    check(wait_until([&] { return a.peers().empty(); }), "A sees B leave");
    Raw b_again(port);
    b_again.send(ctl::login(kMacB, "HunterB", kProduct));
    check(b.closed(), "a new login with the same MAC closes the older connection");
    Raw bad(port);
    bad.send(ctl::login(Mac{}, "Nobody", kProduct));
    check(bad.closed(), "a login with an empty MAC is refused");

    check(wait_until(
              [&] {
                  // Players and traffic are published together by the server thread.
                  // Inspect the same fresh snapshot rather than retaining pre-wait data.
                  const ServerStatus status = server.status();
                  return status.players.size() == 2u && status.relayed_packets >= 4u;
              },
              1000),
        "the status lists players and traffic");

    server.stop();
    check(wait_until([&] { return a.server_state() == ServerState::Connecting; }),
        "stopping the server disconnects its players");
    a.stop();
}

void discovery(std::uint16_t port) {
    Discovery &discovery = Discovery::get();
    discovery.start_announcing(port, [] {
        Announcement info;
        info.name = "Test host";
        info.product = kProduct;
        info.players = 3;
        return info;
    });
    // A query to the host's port is answered with its announcement.
    const Socket s = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    set_nonblocking(s);
    const Address host = Address::ipv4_address(htonl(INADDR_LOOPBACK), port);
    std::string answer;
    wait_until([&] {
        const std::string query = std::string("YKAH") + '\x01' + '\x02';
        ::sendto(s, query.data(), static_cast<int>(query.size()), 0, reinterpret_cast<const sockaddr *>(&host.storage),
            host.length);
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        char buffer[128];
        const auto count = ::recv(s, buffer, static_cast<int>(sizeof(buffer)), 0);
        if (count > 0) answer.assign(buffer, static_cast<std::size_t>(count));
        return !answer.empty();
    });
    check(answer.size() == 58u && answer.compare(0, 4, "YKAH") == 0 && answer[5] == 1 &&
            wire::get16(answer.data() + 6) == port && static_cast<std::uint8_t>(answer[10]) == 3u &&
            wire::get_fixed(answer.data() + 16, 10) == kProduct &&
            wire::get_fixed(answer.data() + 26, 32) == "Test host",
        "a query is answered with the host's announcement");

    // An announcement sent to the discovery port is listed.
    discovery.start_listening();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    std::string packet = answer;
    // Flip a bit so the peer session always differs from our random session.
    packet[12] = static_cast<char>(static_cast<unsigned char>(packet[12]) ^ 0x02u);
    packet.replace(26, 32, std::string("Other host") + std::string(22, '\0'));
    const Address listener = Address::ipv4_address(htonl(INADDR_LOOPBACK), kDiscoveryPort);
    bool found = false;
    wait_until([&] {
        ::sendto(s, packet.data(), static_cast<int>(packet.size()), 0,
            reinterpret_cast<const sockaddr *>(&listener.storage), listener.length);
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        for (const FoundHost &entry : discovery.hosts())
            if (entry.info.name == "Other host" && entry.address == "127.0.0.1" && entry.info.players == 3u)
                found = true;
        return found;
    });
    check(found, "a heard announcement is listed with its address");
    close_socket(s);
    discovery.stop_announcing();
    // Informational: what the host screen would offer on this machine.
    for (const LocalAddress &address : local_addresses())
        std::cout << "     reachable at " << address.address << " (" << address.interface << ", " << address.network
                  << ")" << std::endl;
}

// A process that hosts, with a player in a group and discovery running, and
// then leaves main(): with `tidy`, after shutting the network down as the
// game does; without, leaving everything to exit, which must not abort either.
int exit_while_hosting(bool tidy, bool owned = false) {
    // Match HostState in host.cpp: discovery may ask for the player count
    // until process exit, so its server must remain alive for that interval.
    static Server *process_server = new Server;
    struct OwnedHost {
        Server server;
        ~OwnedHost() {
            // An owner that frees its server must first join callback users.
            Discovery::get().shutdown();
            Client::get().shutdown();
        }
    };
    Server &server = owned ? []() -> Server & {
        static OwnedHost owned_host;
        return owned_host.server;
    }()
        : *process_server;
    const std::uint16_t port = start_server(server);
    if (port == 0u) return 3;
    Discovery::get().start_listening();
    Discovery::get().start_announcing(port, [&server] {
        Announcement info;
        info.name = "Exiting host";
        info.product = kProduct;
        info.players = static_cast<unsigned>(server.status().players.size());
        return info;
    });
    Client &client = Client::get();
    Identity identity;
    identity.server = "127.0.0.1:" + std::to_string(port);
    identity.nickname = "Leaver";
    identity.mac = kMacA;
    identity.product = kProduct;
    client.start(identity);
    client.join(kGroup);
    if (!wait_until([&] { return client.in_group(); })) return 3;
    (void)client.pdp_open(10000, 8192);
    std::this_thread::sleep_for(std::chrono::milliseconds(1500)); // announcing, relaying
    if (tidy) {
        Discovery::get().shutdown();
        server.stop();
        client.shutdown();
    }
    return 0;
}

// Runs this program again in one of the exit modes; true when it exited with 0.
bool exits_cleanly(const char *self, const char *mode) {
    const std::string command = std::string("\"") + self + "\" " + mode;
    const int status = std::system(command.c_str());
    if (status != 0) std::cerr << "child mode " << mode << " returned status " << status << std::endl;
    return status == 0;
}

} // namespace

int main(int argc, char **argv) {
    WinsockSession winsock;
    if (argc > 1 && std::string(argv[1]) == "--exit-while-hosting") return exit_while_hosting(false);
    if (argc > 1 && std::string(argv[1]) == "--exit-after-shutdown") return exit_while_hosting(true);
    if (argc > 1 && std::string(argv[1]) == "--exit-with-owned-server") return exit_while_hosting(false, true);
    check(exits_cleanly(argv[0], "--exit-after-shutdown"), "a hosting process that shuts down exits with 0");
    check(exits_cleanly(argv[0], "--exit-with-owned-server"),
        "an owned host joins callback users before freeing its server");
    // The race this guards against, a thread using what exit() already
    // destroyed, does not show every time.
    bool clean = true;
    for (int run = 0; run < 5 && clean; ++run) clean = exits_cleanly(argv[0], "--exit-while-hosting");
    check(clean, "a hosting process that just returns exits with 0, five times");
    server_and_client();
    discovery(37500);
    std::cout << (failures == 0 ? "all ad hoc tests passed" : std::to_string(failures) + " failed") << std::endl;
    return failures == 0 ? 0 : 1;
}
