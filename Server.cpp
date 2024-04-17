#include <iostream>
#include <string>
#include <regex>
#include <thread>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "Client-Handler.cpp"

#pragma comment(lib, "Ws2_32.lib")

std::mutex mtx;

class Server;  // Forward declaration of Server

class ServerHandler {
public:
    SOCKET clientSocket;
    int clientNumber;
    std::string username;
    Server* server;  // Add a pointer to the server

    ServerHandler(SOCKET socket, int number, Server* server, const std::string& username)
       : clientSocket(socket), clientNumber(number), server(server), username(username) {}


    void handleClient();  // Declare the method here
};

class Server {
private:
    SOCKET listenSocket;
    std::vector<ServerHandler*> clientHandlers;
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
        for (auto* handler : clientHandlers) {
            delete handler;  // Delete the handler
        }
    }

    void broadcastMessage(const std::string& message) {
        for (auto* handler : clientHandlers) {
            send(handler->clientSocket, message.c_str(), static_cast<int>(message.length()), 0);
        }
    }

    void startClient() {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cerr << "WSAStartup failed." << std::endl;
            return;
        }

        std::vector<ClientHandler*> allClients;
        SOCKET socket = INVALID_SOCKET; // Placeholder for actual socket initialization
        ClientHandler handler(socket, 1, allClients);
        std::thread clientThread(&ClientHandler::run, &handler);
        clientThread.join();

        WSACleanup();
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
            try {
                SOCKET clientSocket = accept(listenSocket, nullptr, nullptr);
                if (clientSocket == INVALID_SOCKET) {
                    std::cerr << "Accept failed with error: " << WSAGetLastError() << std::endl;
                    continue; // Attempt to accept a new connection
                }

                // Receive the username from the client
                char usernameBuffer[1024] = { 0 };
                int bytesReceived = recv(clientSocket, usernameBuffer, sizeof(usernameBuffer), 0);
                if (bytesReceived == SOCKET_ERROR || bytesReceived == 0) {
                    std::cerr << "Failed to receive username from client" << std::endl;
                    continue;
                }
                std::string username(usernameBuffer, bytesReceived);

                std::cout << "New client #" << clientNumber << " connected." << std::endl;
                ServerHandler* handler = new ServerHandler(clientSocket, clientNumber, this, username);
                clientHandlers.push_back(handler);  // Store the handler
                std::thread(&ServerHandler::handleClient, handler).detach();  // Start the handler in a new thread
                clientNumber++;
            }
            catch (const std::exception& e) {
                std::cerr << "Server error: " << e.what() << std::endl;
            }
        }
    }
};

// Define the method after the Server class is fully defined
void ServerHandler::handleClient() {
    char buffer[1024] = { 0 };
    while (true) {
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived == SOCKET_ERROR || bytesReceived == 0) {
            std::cout << "Client " << username << " disconnected.\n";
            break;
        }
        std::string message = username + ": " + std::string(buffer, bytesReceived);
        server->broadcastMessage(message);  // Send the message to the server for broadcasting
    }
}

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

        // Start the client after the server has started
        server.startClient();  // Call startClient() on the server instance
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
