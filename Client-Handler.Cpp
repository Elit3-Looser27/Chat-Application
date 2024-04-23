#define NOMINMAX
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
#include "Client-Handler.h"


#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "libcrypto.lib")
#pragma comment(lib, "libssl.lib")

extern std::mutex mtx;
std::string decrypt(const std::string& cipherText, int shift) {
    std::string result = "";
    for (char c : cipherText) {
        if (isalpha(c)) {
            char base = islower(c) ? 'a' : 'A';
            result += char((c - base - shift + 26) % 26 + base);
        }
        else {
            result += c; // Non-alphabetical characters are not decrypted
        }
    }
    return result;
}

//class ClientHandler {
//    SOCKET clientSocket;
//    int clientNumber;
//    std::string username;
//    std::vector<ClientHandler*>& allClients;
//    static const int NUMBER_OF_MESSAGES_TO_SEND = 15;
//
//public:
ClientHandler::ClientHandler(SOCKET socket, int number, std::vector<ClientHandler*>& clients)
    : clientSocket(socket), clientNumber(number), allClients(clients) {
    std::cout << "New connection with user " << username << " at socket " << socket << std::endl;
}

void ClientHandler::run() {
    try {
        if (authenticateUser()) {
            enterChatRoom();
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error handling client# " << clientNumber << ": " << e.what() << std::endl;
    }
    // This will be executed whether an exception was thrown or not
    closeConnection();
}



bool ClientHandler::authenticateUser() {
    std::string option;
    std::string username, password, displayName;
    bool authenticated = false;

    while (!authenticated) {
        std::cout << "Type 'signup' to register or 'login' to access your account: ";
        std::cin >> option;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');  // Ignore newline


        if (option == "signup") {
            std::cout << "Choose a username: ";
            std::cin >> username;
            std::cout << "Choose a password: ";
            std::cin >> password;
            authenticated = registerUser(username, password);
        }
        else if (option == "login") {
            std::cout << "Username: ";
            std::cin >> username;
            std::cout << "Password: ";
            std::cin >> password;
            authenticated = loginUser(username, password);
            if (authenticated) {
                std::cout << "Please Type in your Display name: ";
                std::cin >> displayName;
            }
            else {
                std::cout << "Login failed. Please try again or sign up.\n";
            }
        }
    }

    this->displayName = displayName;
    return authenticated;
}



bool ClientHandler::registerUser(const std::string& username, const std::string& password) {
    // Check if a user with the same username already exists
    std::ifstream in("credentials.txt");
    std::string usr, pwd;
    while (in >> usr >> pwd) {
        if (usr == username) {
            std::cout << "A user with the same username already exists.\n";
            return false;
        }
    }
    in.close();

    std::ofstream out("credentials.txt", std::ios::app);
    if (out) {
        // Encrypt the password before saving it
        std::string encryptedPassword = encrypt(password, 3);
        out << username << " " << encryptedPassword << "\n";
        out.close();
        this->username = username;
        std::cout << "User " << this->username << " registered successfully." << std::endl << std::endl << "Please Type in your Display name " << std::endl;;
        return true;
    }
    return false;
}

bool ClientHandler::loginUser(const std::string& username, const std::string& password) {
    std::ifstream in("credentials.txt");
    std::string usr, pwd;
    // Encrypt the input password before comparing it with the saved password
    std::string encryptedPassword = encrypt(password, 3);
    while (in >> usr >> pwd) {
        if (usr == username && pwd == encryptedPassword) {
            this->username = username;
            std::cout << "User " << this->username << " logged in successfully." << std::endl << std::endl << "Please Type in your Display name " << std::endl;;
            return true;
        }
    }
    in.clear(); // Clear the end-of-file flag
    in.seekg(0); // Move the read position to the beginning of the file
    return false;
}



// Add the encrypt and decrypt functions to the ClientHandler class
std::string ClientHandler::encrypt(std::string text, int key) {
    std::string result = "";

    for (int i = 0; i < text.length(); i++) {
        if (isupper(text[i]))
            result += char(int(text[i] + key - 65) % 26 + 65);
        else
            result += char(int(text[i] + key - 97) % 26 + 97);
    }

    return result;
}

/*std::string decrypt(std::string cipherText, int key) {
    std::string result = "";

    for (int i = 0; i < cipherText.length(); i++) {
        if (isupper(cipherText[i]))
            result += char(int(cipherText[i] - key - 65 + 26) % 26 + 65);
        else
            result += char(int(cipherText[i] - key - 97 + 26) % 26 + 97);
    }

    return result;
}*/

void ClientHandler::enterChatRoom() {
    std::string message, recipientUsername;
    int key = 3;  // You can choose any key
    while (true) {
        std::cout << "Enter recipient's username: ";
        std::getline(std::cin, recipientUsername);
        std::cout << "Enter your message: ";
        std::getline(std::cin, message);
        if (message == "exit") break;

        std::string encryptedMessage = encrypt(message, key);
        /*std::cout << "Encrypted message: " << encryptedMessage << std::endl;*/
        std::string messageWithUsername = this->username + ": " + encryptedMessage;
        saveMessageToFile(messageWithUsername);  // Save the encrypted message
        broadcastMessageToClients(messageWithUsername);
    }
}

void ClientHandler::saveMessageToFile(const std::string& message) {
    std::ofstream out("encrypted_messages.txt", std::ios::app);
    if (out) {
        out << message << std::endl;  // Save the encrypted message to file
        out.close();
    }
}



void ClientHandler::broadcastMessageToClients(const std::string& message) {
    std::lock_guard<std::mutex> lock(mtx);
    for (auto* client : allClients) {
        if (client->isAuthenticated()) {
            std::string messageWithUsername = this->username + ": " + message;
            std::cout << "Broadcasting to user " << client->username << std::endl;
            // Simulate sending a message
            send(client->clientSocket, messageWithUsername.c_str(), messageWithUsername.size(), 0);
        }
    }
}

bool ClientHandler::isAuthenticated() const {
    return !username.empty();
}

void ClientHandler::closeConnection() {
    closesocket(clientSocket);
    std::cout << "Connection with user" << displayName << " closed" << std::endl;
}