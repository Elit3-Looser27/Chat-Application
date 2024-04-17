#include <iostream>
#include <string>
#include <regex>
#include <thread>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

class Client {
private:
    public:
        bool isValidAddress(const std::string& address) {
            std::regex ipPattern(R"((?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?))");
            return std::regex_match(address, ipPattern);
        }
    SOCKET socket_fd;



    void startListenerThread() {
        std::thread([this]() {
            try {
                char buffer[1024] = { 0 };
                while (true) {
                    int bytes_received = recv(this->socket_fd, buffer, sizeof(buffer), 0);
                    if (bytes_received == SOCKET_ERROR || bytes_received == 0) {
                        throw std::runtime_error("Connection to the server lost or closed.");
                    }
                    std::string response(buffer, bytes_received);
                    std::cout << response << std::endl;
                }
            }
            catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << std::endl;
                exit(0);
            }
            }).detach();
    }

public:
    Client() : socket_fd(INVALID_SOCKET) {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            throw std::runtime_error("WSAStartup failed");
        }
    }

    ~Client() {
        if (socket_fd != INVALID_SOCKET) {
            closesocket(socket_fd);
        }
        WSACleanup();
    }

    void connectToServer(const std::string& serverAddress, int port) {
        struct sockaddr_in server;

        if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
            throw std::runtime_error("Socket creation failed with error");
        }

        server.sin_family = AF_INET;
        server.sin_port = htons(port);

        if (inet_pton(AF_INET, serverAddress.c_str(), &server.sin_addr) <= 0) {
            throw std::invalid_argument("Invalid address / Address not supported");
        }

        if (connect(socket_fd, reinterpret_cast<struct sockaddr*>(&server), sizeof(server)) == SOCKET_ERROR) {
            closesocket(socket_fd);
            throw std::runtime_error("Connection failed with error");
        }

        std::cout << "Server running on [" << serverAddress << ":" << port << "]\n";
        std::cout << "Type 'exit' to quit\n";

        startListenerThread();
    }

    void sendMessage() {
        std::string message;
        while (true) {
            std::getline(std::cin, message);
            if (message == "exit") {
                break;
            }
            send(socket_fd, message.c_str(), static_cast<int>(message.length()), 0);
        }
    }
};

int main() {
    try {
        Client client;
        std::string serverAddress;
        int port;

        std::cout << "Enter server address: ";
        std::getline(std::cin, serverAddress);
        while (!client.isValidAddress(serverAddress)) {
            std::cout << "Invalid server address, try again: ";
            std::getline(std::cin, serverAddress);
        }

        std::cout << "Enter port: ";
        std::cin >> port;
        std::cin.ignore(); // Ignore newline after the integer input

        client.connectToServer(serverAddress, port);
        client.sendMessage();
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
