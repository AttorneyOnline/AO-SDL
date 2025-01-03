#include "NetworkThread.h"

#include "ao/net/AOClient.h"
#include "ao/net/PacketTypes.h"
#include "utils/Log.h"

// todo: instead of passing the socket through the constructor, NetworkThread should have thread-safe mechanisms to
// manage the lifecycle of the underlying sockets
NetworkThread::NetworkThread(WebSocket& sock) : sock(sock), net_thread(&NetworkThread::net_loop, this) {
}

void NetworkThread::net_loop() {
    // todo: define a more generalized API for network backends that AOClient implements
    // should do this later so the requirements are understood better
    // once this is done, we can even load network backends dynamically from shared libs(!!)
    // this same generalization should be done to the WebSocket API
    // the current public API is just a read, write, (blocking) connect, and is_connected()
    AOClient ao_client;
    bool fuk = false;

    if (!sock.is_connected()) {
        // todo: this function is blocking
        // should implement a timeout
        sock.connect();
        ao_client.conn_state = CONNECTED;
    }

    std::vector<WebSocket::WebSocketFrame> msgs;
    std::string msgstr;

    while (true) {
        while (msgs.size() < 1) {
            // todo: cleanly handle and stitch continuation packets
            msgs = sock.read();
            // todo: make this timeout a bit less stupid
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }

        for (auto msg : msgs) {
            msgstr = std::string(msg.data.begin(), msg.data.end());
            Log::log_print(DEBUG, "SERVER: %s", msgstr.c_str());
            ao_client.handle_message(msgstr);
        }
        msgs.clear();

        std::vector<std::string> client_msgs = ao_client.get_messages();
        for (auto cli_msg : client_msgs) {
            std::vector<uint8_t> msg_bytes(cli_msg.begin(), cli_msg.end());
            Log::log_print(DEBUG, "CLIENT: %s", cli_msg.c_str());
            sock.write(msg_bytes);
        }

        // todo: add interface to get events from AOClient, add event framework (lol), pass events into queue with locks
        // auto events = ao_client.get_client_events();
        // event_queue.push(events);
    }
}