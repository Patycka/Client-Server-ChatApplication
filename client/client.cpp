#include "sockpp/tcp_connector.h"
#include "sockpp/tcp_socket.h"

using namespace std;

// receive tcp messages from the server (keep reading message, client list message/ping message) - for the future check if message is a 
// ping message then ignore or list message, parsing the json array
// receive udp messages from other clients (just to test different kind of protocol)
// send udp message to other clients


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

void receiveTcpMessages(sockpp::tcp_socket sock)
{
    std::string length;
    std::string message;
    sockpp::result<size_t> res;

    while ((length += readBytes(sock, 1)), (length.back() != ':')) {

    }
    length.erase(length.end() - 1);
    message = readBytes(sock, std::stoi(length));

    
}

int main(int argc, char* argv[]) {

    cout << "Sample TCP echo server for 'sockpp' " << sockpp::SOCKPP_VERSION << '\n' << endl;

    return 0;
}