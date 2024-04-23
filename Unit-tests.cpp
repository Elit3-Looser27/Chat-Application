#include <gtest/gtest.h>
#include "Server.h"
#include "Client.h"
#include "Client-Handler.h"

// Mock classes or real implementations would be included here
// #include "mock_server.h" 
// #include "mock_client.h"

// Test fixture for Server
class ServerTest : public ::testing::Test {
protected:
    Server* server;

    void SetUp() override {
        server = new Server();
        // Assuming Server can be initialized with IP and port directly for testing
        server->start("127.0.0.1", 5000);
    }

    void TearDown() override {
        delete server;
    }
};

// Test fixture for Client
class ClientTest : public ::testing::Test {
protected:
    Client* client;

    void SetUp() override {
        client = new Client();
        client->connectToServer("127.0.0.1", 5000);
    }

    void TearDown() override {
        delete client;
    }
};

// Tests that the server starts correctly
TEST_F(ServerTest, StartsAndListens) {
    ASSERT_TRUE(server->isListening()) << "Server should be listening after start";
}

// Tests client connection
TEST_F(ClientTest, ConnectsToServer) {
    ASSERT_TRUE(client->isConnected()) << "Client should be connected to server";
}

// Tests user registration
TEST_F(ClientTest, UserRegistration) {
    bool result = client->registerUser("newuser", "password123");
    ASSERT_TRUE(result) << "User should be registered successfully";
}

// Tests message sending
TEST_F(ClientTest, SendMessage) {
    bool result = client->sendMessage("Hello World");
    // Assuming client can confirm the message was sent
    ASSERT_TRUE(result) << "Message should be sent successfully";
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
