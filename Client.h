#pragma once
#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include <vector>
#include "Client-Handler.h"

class Client {
private:
    std::vector<ClientHandler*> allClients;
    std::string displayName;
public:
    std::string username;
    SOCKET socket_fd;

    Client();
    ~Client();
    bool isValidAddress(const std::string& address);
    void startListenerThread();
    void connectToServer(const std::string& serverAddress, int port);
    void sendMessage();
    std::string encrypt(const std::string& text, int shift, bool shouldEncrypt = true);
    void listMessages();
};

#endif // CLIENT_H
