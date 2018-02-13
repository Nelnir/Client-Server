#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "server.h"
#include "client.h"
#include <thread>
#include <vector>
using namespace std::chrono_literals;
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
    MOCK_METHOD0(onDisconnected, void());
    MOCK_METHOD0(onServerWrongPassword, void());
    MOCK_METHOD1(onArgumentsError, void(const char*));
    MOCK_METHOD0(onUnableToConnect, void());
    MOCK_METHOD0(onServerIsFull, void());
    MOCK_METHOD0(onBlockedFromServer, void());
    MOCK_METHOD1(onError, void(const std::string&));
    MOCK_METHOD0(onServerPasswordNeeded, std::string());

    MOCK_METHOD3(onMessageReceived, void(const std::string&, const std::string&, const ClientType& l_type));
    MOCK_METHOD1(onServerMessageReceived, void(const std::string&));
    MOCK_METHOD0(onKick, void());
    MOCK_METHOD2(onPromotion, void(const std::string&, const bool&));
    MOCK_METHOD2(onConnectionNotificationReceived, void(const std::string&, const Type&));
    MOCK_METHOD0(onServerExit, void());
};

class ServerClientTest : public testing::Test
{
    virtual void SetUp(){
        t_server = nullptr;
    }
    virtual void TearDown(){
        for(auto& itr : m_clients){
            if(itr.second){
                if(itr.second->joinable()) itr.second->join();
                delete itr.second;
            }
             delete itr.first;
        }
        m_clients.clear();


        if(t_server->joinable()) t_server->join();
        delete t_server;
    }
protected:
    MockServer m_server;
    std::thread* t_server;
    std::vector<std::pair<MockClient*, std::thread*>> m_clients;

    void startServer(const sf::Uint16& l_port,
                     const std::chrono::milliseconds& time = 100ms,
                     const sf::Uint32& l_max = -1,
                     const std::string& l_password = ""){
        m_server.setPort(l_port);
        m_server.setPassword(l_password);
        m_server.setMaxNumberOfClients(l_max);
        t_server = new std::thread(&MockServer::run, &m_server);
        std::thread([&](){ std::this_thread::sleep_for(time); m_server.quit();}).detach();
    }
    bool startClient(const sf::Uint16& l_port,
                     const sf::IpAddress& l_ip,
                     const std::string& l_nick,
                     const std::chrono::milliseconds& time = 200ms,
                     const bool& l_run = false,
                     const std::string& l_password = ""){
        m_clients.emplace_back(std::make_pair(new MockClient, nullptr));
        m_clients.back().first->setNickname(l_nick);
        Status status = m_clients.back().first->connect(l_port, l_ip, l_password);
        if(l_run && status == Status::Connected){
            m_clients.back().second = new std::thread(&MockClient::run, m_clients.back().first);
            std::thread([&time](MockClient* client){ std::this_thread::sleep_for(time); client->quit();}, m_clients.back().first).detach();
            return true;
        }
        return false;
    }
};

TEST_F(ServerClientTest, ConnectingAndDisconnecting)
{
    EXPECT_CALL(m_server, onClientConnected(testing::_)).Times(1);
    EXPECT_CALL(m_server, onClientDisconnected(testing::_)).Times(1);
    startServer(53000, 25ms);
    startClient(53000, "localhost", "marcin");
}

TEST_F(ServerClientTest, ConnectingToServerWithAPassword)
{
    EXPECT_CALL(m_server, onClientConnected(testing::_)).Times(1);
    EXPECT_CALL(m_server, onClientDisconnected(testing::_)).Times(1);
    startServer(53000, 25ms, -1, "pass");
    startClient(53000, "localhost", "marcin", 0ms, false, "pass");
}

TEST_F(ServerClientTest, SendingMessages)
{
    EXPECT_CALL(m_server, onClientConnected(testing::_)).Times(testing::AnyNumber());
    EXPECT_CALL(m_server, onClientDisconnected(testing::_)).Times(testing::AnyNumber());
    EXPECT_CALL(m_server, onClientMessageReceived(testing::_, "siema"));
    startServer(53000, 350ms);
    EXPECT_TRUE(startClient(53000, "localhost", "marcin", 150ms, true));

    EXPECT_CALL(*m_clients.back().first, onConnectionNotificationReceived("nelnir", Type::Connection));

    EXPECT_TRUE(startClient(53000, "localhost", "nelnir", 250ms, true));

    EXPECT_CALL(*m_clients.back().first, onMessageReceived("siema", "marcin", testing::_));
    EXPECT_CALL(*m_clients.back().first, onConnectionNotificationReceived("marcin", Type::Disconnection));

    m_clients.front().first->sendToServer("siema");
}

TEST_F(ServerClientTest, RecevingMessageFromServer)
{
    EXPECT_CALL(m_server, onClientConnected(testing::_)).Times(testing::AnyNumber());
    EXPECT_CALL(m_server, onClientDisconnected(testing::_)).Times(testing::AnyNumber());
    startServer(53000, 250ms);
    EXPECT_TRUE(startClient(53000, "localhost", "marcin", 150ms, true));
    EXPECT_CALL(*m_clients.front().first, onServerMessageReceived("testing"));
    EXPECT_CALL(*m_clients.front().first, onServerExit()).Times(testing::AnyNumber());

    m_server.sendMessageToAllClients("testing");
}

TEST_F(ServerClientTest, PromotingClients)
{
    EXPECT_CALL(m_server, onClientConnected(testing::_)).Times(testing::AnyNumber());
    EXPECT_CALL(m_server, onClientDisconnected(testing::_)).Times(testing::AnyNumber());

    startServer(53000, 250ms);
    EXPECT_CALL(m_server, onClientPromoted(testing::_, testing::_)).Times(testing::AnyNumber());

    EXPECT_TRUE(startClient(53000, "localhost", "marcin", 150ms, true));
    EXPECT_EQ(m_clients.front().first->getType(), ClientType::Normie);
    EXPECT_CALL(*m_clients.front().first, onPromotion(testing::_, true));
    m_server.promote("localhost", ClientType::Administrator);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(m_clients.front().first->getType(), ClientType::Administrator);
}
