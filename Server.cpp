#include <iostream>
#include <string>
#include <regex>
#include <thread>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

class ClientHandler {
public:
    SOCKET clientSocket;
    int clientNumber;
    ClientHandler(SOCKET socket, int number) : clientSocket(socket), clientNumber(number) {}

    void handleClient() {
        char buffer[1024] = { 0 };
        while (true) {
            int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
            if (bytesReceived == SOCKET_ERROR || bytesReceived == 0) {
                std::cout << "Client #" << clientNumber << " disconnected.\n";
                closesocket(clientSocket);
                break;
            }
            std::cout << "Client #" << clientNumber << ": " << std::string(buffer, bytesReceived) << std::endl;
        }
    }
};

class Server {
private:
    SOCKET listenSocket;
    std::vector<std::thread> clientThreads;

public:
    Server() : listenSocket(INVALID_SOCKET) {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            throw std::runtime_error("WSAStartup failed");
        }
    }

    ~Server() {
        WSACleanup();
        for (auto& th : clientThreads) {
            if (th.joinable()) {
                th.join();
            }
        }
    }

    void start(const std::string& address, int port) {
        struct sockaddr_in serverAddr;
        listenSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (listenSocket == INVALID_SOCKET) {
            throw std::runtime_error("Socket creation failed");
        }

        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        inet_pton(AF_INET, address.c_str(), &serverAddr.sin_addr);

        if (bind(listenSocket, reinterpret_cast<struct sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
            closesocket(listenSocket);
            throw std::runtime_error("Bind failed");
        }

        if (listen(listenSocket, 5) == SOCKET_ERROR) {
            closesocket(listenSocket);
            throw std::runtime_error("Listen failed");
        }

        std::cout << "Server running on " << address << ":" << port << std::endl;

        int clientNumber = 0;
        while (true) {
            SOCKET clientSocket = accept(listenSocket, nullptr, nullptr);
            if (clientSocket == INVALID_SOCKET) {
                std::cerr << "Accept failed" << std::endl;
                continue;
            }
            std::cout << "New client #" << clientNumber << " connected." << std::endl;
            clientThreads.emplace_back([clientSocket, clientNumber]() {
                ClientHandler handler(clientSocket, clientNumber);
                handler.handleClient();
                });
            clientNumber++;
        }
    }
};

int main() {
    try {
        Server server;
        std::string serverAddress;
        int port;

        std::cout << "Enter server address: ";
        std::getline(std::cin, serverAddress);

        std::cout << "Enter port: ";
        std::cin >> port;

        server.start(serverAddress, port);
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}
