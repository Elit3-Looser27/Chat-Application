#pragma once
#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <vector>
#include "Client-Handler.h"

class ServerHandler;

class Server {
private:
    SOCKET listenSocket;
    std::vector<ServerHandler*> clientHandlers;
    std::vector<std::thread> clientThreads;

public:
    Server();
    ~Server();
    void broadcastMessage(const std::string& message, SOCKET senderSocket);
    void startClient();
    void start(const std::string& address, int port);
};

#endif // SERVER_H
