#define NOMINMAX
#include <iostream>
#include <string>
#include <regex>
#include <thread>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "Client-Handler.cpp"
#include <limits>

#pragma comment(lib, "Ws2_32.lib")


std::mutex mtx;

class Client {
private:
    std::vector<ClientHandler*> allClients;
    /*std::string username;*/
    std::string displayName;
public:
    std::string username;
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



    void listMessages() {
        std::ifstream in("Client/stored_messages.txt");
        if (!in) {
            std::cerr << "Failed to open stored_messages.txt" << std::endl;
            return;
        }

        std::string line;
        int key = 3;  // The same key as used in encryption
        while (std::getline(in, line)) {
            // Separate the display name from the message
            size_t pos = line.find("): ");
            if (pos == std::string::npos) {
                std::cerr << "Invalid message format" << std::endl;
                continue;
            }
            std::string displayName = line.substr(0, pos);  // Extract the display name
            std::string encryptedMessage = line.substr(pos + 3);  // Extract the encrypted message

            std::string decryptedMessage = decrypt(encryptedMessage, key);  // Decrypt the message
            std::cout << displayName << ": " << decryptedMessage << std::endl;
        }
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
        std::cout << "Type 'list messages' to print the chat logs\n";

        startListenerThread();

        ClientHandler handler(socket_fd, 0, allClients);  // Create a ClientHandler instance
        username = handler.authenticateUser();// Create a ClientHandler instance
        if (username.empty()) {  // If the returned username is empty, authentication failed
            throw std::runtime_error("Authentication failed");
        }
        /*std::cout << "Enter your display name: " << std::endl;
        std::getline(std::cin, displayName);*/
    }



    void sendMessage() {
        std::string message;
        int key = 3;  // Define the key for the Caesar cipher

        while (true) {
            std::cout << "Enter your message: " << std::endl;
            std::getline(std::cin, message);
            if (message == "exit") {
                break;
            }

            if (message == "list messages") {
                listMessages();
                continue;
            }

            std::string encryptedMessage = encrypt(message, key); // Encrypt the message
            std::string messageWithUsername = username + " (" + username + "): " + encryptedMessage; // Use displayName directly

            send(socket_fd, messageWithUsername.c_str(), static_cast<int>(messageWithUsername.length()), 0);
        }
    }


    // Utility function to perform the Caesar cipher encryption
    std::string encrypt(const std::string& text, int shift, bool shouldEncrypt = true) {
        std::string result = "";
        for (char c : text) {
            if (shouldEncrypt && isalpha(c)) {
                char base = islower(c) ? 'a' : 'A';
                result += char((c - base + shift) % 26 + base);
            }
            else {
                result += c; // Non-alphabetical characters are not encrypted
            }
        }
        return result;
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
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Ignore newline after the integer input

        client.connectToServer(serverAddress, port);
        client.sendMessage();
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}