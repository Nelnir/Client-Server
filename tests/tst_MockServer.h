#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "server.h"
#include "client.h"

using testing::AtLeast;

class MockServer : public Server
{
public:
    MOCK_METHOD1(onClientBlocked, void(std::unique_ptr<ClientServerData>&));
    MOCK_METHOD1(onClientRejected, void(std::unique_ptr<ClientServerData>&));
    MOCK_METHOD1(onClientConnected, void(std::unique_ptr<ClientServerData>&));
    MOCK_METHOD1(onClientDisconnected, void(std::unique_ptr<ClientServerData>&));
    MOCK_METHOD2(onClientMessageReceived, void(std::unique_ptr<ClientServerData>&, const std::string&));
    MOCK_METHOD2(onClientPromoted, void(std::unique_ptr<ClientServerData>&, const bool&));
    MOCK_METHOD1(onErrorWithSendingData, void(std::unique_ptr<ClientServerData>&));
    MOCK_METHOD1(onErrorWithReceivingData, void(std::unique_ptr<ClientServerData>&));
    MOCK_METHOD1(onArgumentsError, void(const char*));
    MOCK_METHOD1(error, void(const std::string&));
};

class MockClient : public Client
{
public:
    MOCK_METHOD0(onInitialization, void());
    MOCK_METHOD0(onSuccessfullyConnected, void());
    MOCK_METHOD0(onErrorWithSendingData, void());
    MOCK_METHOD0(onErrorWithReceivingData, void());
    MOCK_METHOD0(onServerClosedConnection, void());
    MOCK_METHOD0(onServerWrongPassword, void());
    MOCK_METHOD1(onArgumentsError, void(const char*));
    MOCK_METHOD0(onUnableToConnect, void());
    MOCK_METHOD0(onServerIsFull, void());
    MOCK_METHOD0(onBlockedFromServer, void());
    MOCK_METHOD1(onError, void(const std::string&));
    MOCK_METHOD0(onServerPasswordNeeded, std::string());

    MOCK_METHOD2(onMessageReceived, void(const std::string&, const ClientType& l_type));
    MOCK_METHOD1(onServerMessageReceived, void(const std::string&));
    MOCK_METHOD0(onKick, void());
    MOCK_METHOD2(onPromotion, void(const std::string&, const bool&));
    MOCK_METHOD2(onConnectionNotificationReceived, void(const std::string&, const Type&));
    MOCK_METHOD0(onServerExit, void());
};

using namespace testing;

TEST(CaseName, SetName)
{
    MockServer server;
    EXPECT_CALL(server, error(_)).Times(AtLeast(1));
    server.error("xd");
}
