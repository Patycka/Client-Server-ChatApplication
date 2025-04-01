#include <syncstream>
#include "sockpp/tcp_connector.h"
#include "sockpp/tcp_socket.h"
#include "sockpp/udp_socket.h"
#include <thread>
#include "jsoncons/json.hpp"

using namespace std;

// receive tcp messages from the server (keep reading message, client list message/ping message) - 
// for the future check if message is a 
// ping message then ignore or list message, parsing the json array
// receive udp messages from other clients (just to test different kind of protocol)
// send udp message to other clients


std::vector <sockpp::inet_address> g_addresses;
std::mutex g_addressesMutex;

std::string readBytes(sockpp::tcp_socket& sock, size_t n) {
    std::string data(n, '\0'); // Pre-allocate space for n bytes
    size_t total_read = 0;

    while (total_read < n) {
        sockpp::result<std::size_t> bytes_read = sock.read(&data[total_read], n - total_read);
        if (bytes_read) {  // Check if the result is valid (no error)
            total_read += bytes_read.value();
        } else if (bytes_read.value() == 0) {
            throw std::runtime_error("Connection closed by peer");
        } else {
            throw std::runtime_error("Socket read error: " + bytes_read.error_message());
        }
    }

    return data;
}

bool isPingMessage(const std::string& message) {
    return message.find("Is transmission ok?") != std::string::npos;
}

std::vector<std::string> ParseJsonArray(const std::string& jsonStr) {
    std::vector<std::string> result;
    
    try {
        jsoncons::json jsonArray = jsoncons::json::parse(jsonStr);

        if (jsonArray.is_array()) {
            for (const auto& item : jsonArray.array_range()) {
                if (item.is_string()) {
                    result.push_back(item.as<std::string>());
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "JSON parsing error: " << e.what() << std::endl;
        return {};
    }

    return result;
}

sockpp::inet_address splitIpPort(const std::string& input)
{
  std::size_t pos{input.find(':')};

  if (pos == std::string::npos || pos == 0 || pos == input.size() - 1) {
    throw std::invalid_argument(
      "Invalid input format: missing or misplaced colon");
  }

  std::string ip{input.substr(0, pos)};       // Extract IP part
  std::string portStr{input.substr(pos + 1)}; // Extract port part

  // Convert port string to integer
  char*         end{};
  unsigned long portNum{std::strtoul(portStr.c_str(), &end, 10)};

  if (*end != '\0' || portNum > 65535) {
    throw std::invalid_argument("Invalid port number");
  }

  return sockpp::inet_address{ip, static_cast<in_port_t>(portNum)};
}


void receiveTcpMessages(sockpp::tcp_socket sock)
{

    std::string length;
    std::string message;
    sockpp::result<size_t> res;

    while(true)
    {
        while ((length += readBytes(sock, 1)), (length.back() != ':')) {

        }
        length.erase(length.end() - 1);
        message = readBytes(sock, std::stoi(length));
        if (!isPingMessage(message))
        {
            vector<std::string> vector = ParseJsonArray(message);
            std::vector<sockpp::inet_address> addresses{};
            for (const std::string& peer : vector) {
              addresses.push_back(splitIpPort(peer));
            }
            std::unique_lock<std::mutex> lock{g_addressesMutex};
            g_addresses = addresses;
            lock.unlock();
        }
        std::osyncstream(std::cout) << "Message from server: " << message << std::endl;
        length.clear();
    }

}

void receive_udp_messages(sockpp::udp_socket& udp_sock) {
    char buffer[1024];
    sockpp::inet_address sender;

    while (true) {
        auto bytes_read = udp_sock.recv_from(buffer, sizeof(buffer) - 1, &sender);
        // Sleep for 1 second
        std::this_thread::sleep_for(1s);

        if (bytes_read.is_ok() && bytes_read.value() != -1 ) {
            buffer[bytes_read.value()] = '\0';
            std::cout << "\n[UDP Message from " << sender.to_string() << "]: " << buffer << std::endl;
            std::cout << "Enter message: ";
            std::cout.flush();
        }
        
    }
}

void send_udp_messages(sockpp::udp_socket& udp_sock) {
    std::unique_lock<std::mutex> lock{g_addressesMutex};
    std::vector<sockpp::inet_address> peers{g_addresses};
    lock.unlock();

    for (const sockpp::inet_address& address : peers) {
      sockpp::udp_socket udpSendSocket{};
      sockpp::inet_address peerAddress{address.address(), 12345};
      sockpp::result<sockpp::none> connectResult{
        udpSendSocket.connect(peerAddress)
      };
      if (!connectResult) {
        // SAD
      }
    const std::string message{"Hello!"};
    const sockpp::result<std::size_t> sendResult{udp_sock.send(message)};
    if (!sendResult) {
    // SAD
    }
  }
}

int main(int argc, char* argv[]) {

    cout << "Sample TCP echo client test for 'sockpp' "<< endl;

    in_port_t port = (argc > 1) ? atoi(argv[1]) : sockpp::TEST_PORT;
    string host = (argc > 1) ? argv[1] : "localhost";

    sockpp::initialize();
    auto t_start = std::chrono::high_resolution_clock::now();


    error_code ec;

    sockpp::tcp_connector conn({host, port}, ec);
    sockpp::udp_socket udpSock;
    sockpp::udp_socket udpSockSender;
    if (udpSock.bind(sockpp::inet_address(port)).is_error()) {
        std::osyncstream(std::cout) << "Could not bind to UDP port " << port << ":(\n";
        }

    if (ec) {
        cerr << "Error connecting to server at " << sockpp::inet_address(host, port) << "\n\t"
             << ec.message() << endl;
        return 1;
    }

        
    if (!udpSock) {
        //std::cerr << "Error creating UDP socket: " << udpSock.last_error_str() << std::endl;
        return 1;
    }

    std::thread rdThr(receiveTcpMessages, std::move(conn));

    // Start receiving UDP messages in a separate thread
    std::thread recv_thread(receive_udp_messages, std::ref(udpSock));
    recv_thread.detach();

    //conn.shutdown(SHUT_WR);
    rdThr.join();

    return 0;
}