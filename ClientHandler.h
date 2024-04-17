#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <thread>
#include <mutex>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/evp.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "libcrypto.lib")
#pragma comment(lib, "libssl.lib")

class ClientHandler {
    SOCKET clientSocket;
    int clientNumber;
    std::string username;
    std::string privateKey;
    std::string publicKey;
    std::vector<ClientHandler*>& allClients;
    static const int NUMBER_OF_MESSAGES_TO_SEND = 15;

public:
    ClientHandler(SOCKET socket, int number, std::vector<ClientHandler*>& clients);
    void run();
    bool authenticateUser();
    bool registerUser(const std::string& username, const std::string& password);
    bool loginUser(const std::string& username, const std::string& password);
    bool generateRSAKeys(std::string& publicKey, std::string& privateKey);
    std::string encryptMessage(const std::string& message, const std::string& publicKeyPEM);
    std::string getRecipientPublicKey(const std::string& recipientUsername);
    void enterChatRoom();
    void saveMessageToFile(const std::string& message);
    std::string signMessage(const std::string& message);
    EVP_PKEY* loadPrivateKey(const std::string& pemPrivateKey);
    void broadcastMessageToClients(const std::string& message);
    bool isAuthenticated() const;
    void closeConnection();
};

#endif // CLIENT_HANDLER_H
