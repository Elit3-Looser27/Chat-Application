#pragma once
// Client-Handler.h
#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include <string>
#include <vector>
#include <winsock2.h>

class ClientHandler {
    SOCKET clientSocket;
    int clientNumber;
    std::string username;
    std::vector<ClientHandler*>& allClients;
    static const int NUMBER_OF_MESSAGES_TO_SEND = 15;

public:
    ClientHandler(SOCKET socket, int number, std::vector<ClientHandler*>& clients);
    void run();
    bool authenticateUser();
    bool registerUser(const std::string& username, const std::string& password);
    bool loginUser(const std::string& username, const std::string& password);
    std::string encrypt(std::string text, int key);
    void enterChatRoom();
    void saveMessageToFile(const std::string& message);
    void broadcastMessageToClients(const std::string& message);
    bool isAuthenticated() const;
    void closeConnection();
};

std::string decrypt(const std::string& cipherText, int shift);

#endif // CLIENT_HANDLER_H
