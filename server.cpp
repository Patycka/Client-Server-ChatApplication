// tcpechosvr.cpp
//
// A multi-threaded TCP echo server for sockpp library.
// This is a simple thread-per-connection TCP server.
//
// USAGE:
//  	tcpechosvr [port]
//
// --------------------------------------------------------------------------
// This file is part of the "sockpp" C++ socket library.
//
// Copyright (c) 2014-2017 Frank Pagliughi
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
// contributors may be used to endorse or promote products derived from this
// software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
// IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// --------------------------------------------------------------------------

#include <iostream>
#include <thread>
#include <vector>

#include <memory>
#include <mutex>
#include "sockpp/tcp_acceptor.h"
#include "sockpp/tcp_socket.h"
#include "sockpp/version.h"
#include "jsoncons/json.hpp"

using namespace std;
using namespace jsoncons;

std::vector<std::shared_ptr<sockpp::tcp_socket>> clients;
std::mutex list_mutex;


void add_client(std::shared_ptr<sockpp::tcp_socket> client)
{
    std::lock_guard<std::mutex> lock(list_mutex);
    clients.push_back(client);
}

std::vector<std::string> createClientList(const std::vector<std::shared_ptr<sockpp::tcp_socket>>& clients)
{
    // Convert to vector<string>
    std::vector<std::string> list;
    for (const auto& s : clients) {
        list.push_back(s->peer_address().to_string());
    }
    return list;
}

std::string ChangeClientListToJsonArray(const std::vector<std::string>& clientsList)
{
    // Convert to JSON array
    json jsonArray = json::array();
    for (const auto& socket : clientsList) {
        jsonArray.push_back(socket);
    }
    return jsonArray.to_string();
}

std::string CreateLengthPrefixedMessage(const std::string& jsonArray)
{
    // Create length prefixed message (length:jsonArray)
    std::ostringstream oss;
    oss << jsonArray.size() << ":" << jsonArray;
    return oss.str();
}


void SendClientList()
{
    std::vector<std::string> clientsList = createClientList(clients);
    std::string jsonArray = ChangeClientListToJsonArray(clientsList);
    std::string message = CreateLengthPrefixedMessage(jsonArray);


    for (std::size_t i{0}; i < clients.size(); ++i) 
    {
        const auto& socket = clients[i];
        const auto result = socket->write_n(message.c_str(), message.size());
        if (result.is_error()) {
            clients.erase(clients.begin() + i);
            --i;
        }
    }
}

// Thread function
// TCP server: sends a list of clients to all clients: serialize list of client into message
// TCP server that listens for incoming client connections.
// Send a serialized list of clients (in JSON format, for example).
// Send periodic ping messages to keep the connection alive 
void clientHandler(std::shared_ptr<sockpp::tcp_socket> sock)
{
    std::unique_lock<std::mutex> lock(list_mutex);

    SendClientList();

    lock.unlock();

    // Ping this client to check if any client is disconnected
    sockpp::result<std::size_t> ok;
    while (!ok.is_error()) {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(1s);
        std::string message = "Is transmission ok?"; 
        ok = sock->write_n(message.c_str(), message.size());
    }
    // erase this socket from the list of clients
    std::cout << "Connection closed from " << sock->peer_address() << std::endl;
    lock.lock();
    std::erase_if(clients, [&sock](const auto& peer){ return peer->peer_address() == sock->peer_address(); });
    SendClientList();
}

// --------------------------------------------------------------------------
// The thread function. This is run in a separate thread for each socket.
// Ownership of the socket object is transferred to the thread, so when this
// function exits, the socket is automatically closed.

void run_echo(sockpp::tcp_socket sock) {
    char buf[512];
    sockpp::result<size_t> res;

    while ((res = sock.read(buf, sizeof(buf))) && res.value() > 0)
        sock.write_n(buf, res.value());

    cout << "Connection closed from " << sock.peer_address() << endl;
}

// --------------------------------------------------------------------------
// The main thread runs the TCP port acceptor. Each time a connection is
// made a new thread is spawned to handle it leaving this main thread to
// immediately wait for the next incoming connection.

int main(int argc, char* argv[]) {
    cout << "Sample TCP echo server for 'sockpp' " << sockpp::SOCKPP_VERSION << '\n' << endl;

    in_port_t port = (argc > 1) ? atoi(argv[1]) : sockpp::TEST_PORT;

    sockpp::initialize();

    error_code ec;
    sockpp::tcp_acceptor acc{port, 4, ec};

    if (ec) {
        cerr << "Error creating the acceptor: " << ec.message() << endl;
        return 1;
    }
    cout << "Awaiting connections on port " << port << "..." << endl;

    while (true) {
        sockpp::inet_address peer;

        // Accept a new client connection
        if (auto res = acc.accept(&peer); !res) {
            cerr << "Error accepting incoming connection: " << res.error_message() << endl;
        }
        else {
            cout << "Received a connection request from " << peer << endl;

            // accepting code
            std::shared_ptr<sockpp::tcp_socket> socket{
            std::make_shared<sockpp::tcp_socket>(res.release())};
            add_client(socket);

            // Create a thread and transfer the new stream to it.
            thread thr(clientHandler, socket);
            thr.detach();
        }
    }

    return 0;
}
