#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "server.h"
#include "client.h"
#include <thread>
#include <vector>

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

class ServerClientTest : public testing::Test
{
    virtual void SetUp(){
        t_server = nullptr;
    }
    virtual void TearDown(){
        for(auto& itr : m_clients){
            delete itr.first;
            if(itr.second) delete itr.second;
        }
        m_clients.clear();


        if(t_server->joinable()) t_server->join();
        delete t_server;
    }
protected:
    MockServer m_server;
    std::thread* t_server;
    std::vector<std::pair<MockClient*, std::thread*>> m_clients;

    void startServer(const sf::Uint16& l_port, const std::chrono::milliseconds& time = std::chrono::milliseconds(25), const sf::Uint32& l_max = -1, const std::string& l_password = ""){
        m_server.setPort(l_port);
        m_server.setPassword(l_password);
        m_server.setMaxNumberOfClients(l_max);
        t_server = new std::thread(&MockServer::run, &m_server);
        std::thread([&]()
        {
            std::this_thread::sleep_for(time);
            m_server.quit();
        }).detach();
    }
    void startClient(const sf::Uint16& l_port, const sf::IpAddress& l_ip, const std::string& l_nick, const bool& l_run = false, const std::string& l_password = ""){
        m_clients.emplace_back(std::make_pair(new MockClient, nullptr));
        m_clients.back().first->setNickname(l_nick);
        Status status = m_clients.back().first->connect(l_port, l_ip, l_password);
        if(l_run && status == Status::Connected){
            m_clients.back().second = new std::thread(&MockClient::run, m_clients.back().first);
        }
    }
};

TEST_F(ServerClientTest, ConnectingAndDisconnecting)
{
    EXPECT_CALL(m_server, onClientConnected(testing::_)).Times(1);
    EXPECT_CALL(m_server, onClientDisconnected(testing::_)).Times(1);
    startServer(53000);
    startClient(53000, "localhost", "marcin");
}

TEST_F(ServerClientTest, SendingMessages)
{
    EXPECT_CALL(m_server, onClientConnected(testing::_)).Times(testing::AnyNumber());
    EXPECT_CALL(m_server, onClientDisconnected(testing::_)).Times(testing::AnyNumber());
    EXPECT_CALL(m_server, onClientMessageReceived(testing::_, "siema"));
    startServer(53000);
    startClient(53000, "localhost", "marcin");
    m_clients.front().first->sendToServer("siema");
}
