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

extern std::mutex mtx;

class ClientHandler {
    SOCKET clientSocket;
    int clientNumber;
    std::string username;
    std::string privateKey;
    std::string publicKey;
    std::vector<ClientHandler*>& allClients;
    static const int NUMBER_OF_MESSAGES_TO_SEND = 15;

public:
    ClientHandler(SOCKET socket, int number, std::vector<ClientHandler*>& clients)
        : clientSocket(socket), clientNumber(number), allClients(clients) {
        std::cout << "New connection with user " << username << " at socket " << socket << std::endl;
    }

    void run() {
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
    


    bool authenticateUser() {
        std::string option;
        std::string username, password;
        bool authenticated = false;

        while (!authenticated) {
            std::cout << "Type 'signup' to register or 'login' to access your account: ";
            std::cin >> option;

            if (option == "signup") {
                std::cout << "Choose a username: ";
                std::cin >> username;
                std::cout << "Choose a password: ";
                std::cin >> password;
                authenticated = registerUser(username, password);
                if (!authenticated) {
                    std::cout << "Registration failed. Please try again.\n";
                }
            }
            else if (option == "login") {
                std::cout << "Username: ";
                std::cin >> username;
                std::cout << "Password: ";
                std::cin >> password;
                authenticated = loginUser(username, password);
                if (!authenticated) {
                    std::cout << "Login failed. Please try again or sign up.\n";
                }
            }
        }

        return authenticated;
    }



    bool registerUser(const std::string& username, const std::string& password) {
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
            out << username << " " << password << "\n";
            out.close();
            this->username = username;
            std::cout << "User " << this->username << " registered successfully." << std::endl << std::endl << "Please Type in your Display name " << std::endl;;
            return true;
        }
        return false;
    }


    bool loginUser(const std::string& username, const std::string& password) {
        std::ifstream in("credentials.txt");
        std::string usr, pwd;
        while (in >> usr >> pwd) {
            if (usr == username && pwd == password) {
                this->username = username;
                std::cout << "User " << this->username << " logged in successfully." << std::endl << std::endl << "Please Type in your Display name " << std::endl;;
                return true;
            }
        }
        in.clear(); // Clear the end-of-file flag
        in.seekg(0); // Move the read position to the beginning of the file
        return false;
    }


    bool generateRSAKeys(std::string& publicKey, std::string& privateKey) {
        EVP_PKEY* pkey = NULL;
        EVP_PKEY_CTX* pkey_ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);
        if (!pkey_ctx)
            return false;

        if (EVP_PKEY_keygen_init(pkey_ctx) <= 0)
            return false;

        if (EVP_PKEY_CTX_set_rsa_keygen_bits(pkey_ctx, 2048) <= 0)
            return false;

        if (EVP_PKEY_keygen(pkey_ctx, &pkey) <= 0)
            return false;

        // To write the keys into strings we use BIOs
        BIO* bioPublic = BIO_new(BIO_s_mem());
        BIO* bioPrivate = BIO_new(BIO_s_mem());

        if (!PEM_write_bio_PUBKEY(bioPublic, pkey) || !PEM_write_bio_PrivateKey(bioPrivate, pkey, NULL, NULL, 0, NULL, NULL)) {
            EVP_PKEY_free(pkey);
            BIO_free_all(bioPublic);
            BIO_free_all(bioPrivate);
            return false;
        }

        // Extract the keys from the BIOs to the std::string
        size_t pub_len = BIO_pending(bioPublic);
        size_t pri_len = BIO_pending(bioPrivate);
        char* pub_key = new char[pub_len + 1];
        char* pri_key = new char[pri_len + 1];

        BIO_read(bioPublic, pub_key, static_cast<int>(pub_len));
        BIO_read(bioPrivate, pri_key, static_cast<int>(pri_len));
        pub_key[pub_len] = '\0';
        pri_key[pri_len] = '\0';

        publicKey = std::string(pub_key);
        privateKey = std::string(pri_key);

        delete[] pub_key;
        delete[] pri_key;

        EVP_PKEY_free(pkey);
        BIO_free_all(bioPublic);
        BIO_free_all(bioPrivate);

        return true;
    }
    std::string encryptMessage(const std::string& message, const std::string& publicKeyPEM) {
        // Load public key
        BIO* bio = BIO_new_mem_buf((void*)publicKeyPEM.c_str(), -1);
        EVP_PKEY* pubKey = PEM_read_bio_PUBKEY(bio, NULL, NULL, NULL);
        BIO_free(bio);

        // Encrypt the message
        EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(pubKey, NULL);
        EVP_PKEY_encrypt_init(ctx);
        EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING);

        size_t outlen;
        EVP_PKEY_encrypt(ctx, NULL, &outlen, (unsigned char*)message.c_str(), message.length());  // Determine buffer size
        unsigned char* outbuf = (unsigned char*)malloc(outlen);
        if (!outbuf) {
            EVP_PKEY_CTX_free(ctx);
            EVP_PKEY_free(pubKey);
            return "";
        }

        if (EVP_PKEY_encrypt(ctx, outbuf, &outlen, (unsigned char*)message.c_str(), message.length()) <= 0) {
            free(outbuf);
            EVP_PKEY_CTX_free(ctx);
            EVP_PKEY_free(pubKey);
            return "";
        }

        std::string encryptedMessage((char*)outbuf, outlen);
        free(outbuf);
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pubKey);

        return encryptedMessage;
    }
    std::string decryptMessage(const std::string& encryptedMessage) {
        // Load private key
        BIO* bio = BIO_new_mem_buf((void*)privateKey.c_str(), -1);
        EVP_PKEY* privKey = PEM_read_bio_PrivateKey(bio, NULL, NULL, NULL);
        BIO_free(bio);

        // Decrypt the message
        EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(privKey, NULL);
        EVP_PKEY_decrypt_init(ctx);
        EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING);

        size_t outlen;
        EVP_PKEY_decrypt(ctx, NULL, &outlen, (unsigned char*)encryptedMessage.c_str(), encryptedMessage.length());  // Determine buffer size
        unsigned char* outbuf = (unsigned char*)malloc(outlen);
        if (!outbuf) {
            EVP_PKEY_CTX_free(ctx);
            EVP_PKEY_free(privKey);
            return "";
        }

        if (EVP_PKEY_decrypt(ctx, outbuf, &outlen, (unsigned char*)encryptedMessage.c_str(), encryptedMessage.length()) <= 0) {
            free(outbuf);
            EVP_PKEY_CTX_free(ctx);
            EVP_PKEY_free(privKey);
            return "";
        }

        std::string decryptedMessage((char*)outbuf, outlen);
        free(outbuf);
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(privKey);

        return decryptedMessage;
    }



    std::string getRecipientPublicKey(const std::string& recipientUsername) {
        for (auto* client : allClients) {
            if (client->username == recipientUsername) {
                return client->publicKey;
            }
        }
        return ""; // Return an empty string if no public key found
    }

    void enterChatRoom() {
        std::string message, recipientUsername;
        while (true) {
            std::cout << "Enter recipient's username: ";
            std::getline(std::cin, recipientUsername);
            std::cout << "Enter your message: ";
            std::getline(std::cin, message);
            if (message == "exit") break;

            std::string publicKey = getRecipientPublicKey(recipientUsername);
            if (publicKey.empty()) {
                std::cout << "Recipient not found or has no public key." << std::endl;
                continue;
            }

            std::string encryptedMessage = encryptMessage(message, publicKey);
            saveMessageToFile(encryptedMessage);  // Save the encrypted message
            broadcastMessageToClients(encryptedMessage);
        }
    }

    void saveMessageToFile(const std::string& message) {
        std::ofstream out("encrypted_messages.txt", std::ios::app);
        if (out) {
            out << message << std::endl;  // Save the encrypted message to file
            out.close();
        }
    }

    std::string signMessage(const std::string& message) {
        EVP_PKEY* evp_privateKey = loadPrivateKey(this->privateKey);
        EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
        const EVP_MD* md = EVP_sha256();

        EVP_SignInit(mdctx, md);
        EVP_SignUpdate(mdctx, message.c_str(), message.length());

        unsigned char* sig = new unsigned char[EVP_PKEY_size(evp_privateKey)];
        unsigned int siglen;

        EVP_SignFinal(mdctx, sig, &siglen, evp_privateKey);

        std::string signature(reinterpret_cast<char*>(sig), siglen);
        delete[] sig;

        EVP_MD_CTX_free(mdctx);
        EVP_PKEY_free(evp_privateKey);

        return signature;
    }

    EVP_PKEY* loadPrivateKey(const std::string& pemPrivateKey) {
        BIO* bio = BIO_new_mem_buf((void*)pemPrivateKey.c_str(), -1);
        EVP_PKEY* pkey = PEM_read_bio_PrivateKey(bio, NULL, NULL, NULL);
        BIO_free(bio);
        return pkey;
    }

    void broadcastMessageToClients(const std::string& message) {
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

    bool isAuthenticated() const {
        return !username.empty();
    }

    void closeConnection() {
        closesocket(clientSocket);
        std::cout << "Connection with user" << username << " closed" << std::endl;
    }
};

//// Rename main to startClient
//void startClient() {
//    WSADATA wsaData;
//    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
//        std::cerr << "WSAStartup failed." << std::endl;
//        return;
//    }
//
//    std::vector<ClientHandler*> allClients;
//    SOCKET socket = INVALID_SOCKET; // Placeholder for actual socket initialization
//    ClientHandler handler(socket, 1, allClients);
//    std::thread clientThread(&ClientHandler::run, &handler);
//    clientThread.join();
//
//    WSACleanup();
//}
