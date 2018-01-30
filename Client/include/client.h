#ifndef CLIENT_H
#define CLIENT_H

#include <SFML/Network.hpp>
#include "../../Shared/shared.h"
#include <string>
#include <unordered_map>
#include <functional>

enum class Status { ServerIsFull, Connected, WrongPassword, UnableToConnect, Blocked};

using Responses = std::unordered_map<Type, std::function<void(sf::Packet&)>>;

class Client
{
public:
    Client();
    ~Client();

    bool processArguments(int& argc, char**& argv);

    ///SETTERS
    void setPort(const sf::Uint16& l_port) { m_serverPort = l_port; }
    void setIp(const sf::IpAddress& l_ip) { m_serverIp = l_ip; }
    void setNickname(const std::string& l_nick) { m_client.m_name = l_nick; }

    ///GETTERS
    sf::Uint16 getPort() { return m_serverPort; }
    sf::IpAddress getIp() { return m_serverIp; }
    std::string getNickname() { return m_client.m_name; }

    /// MAIN
    bool establishConnection();
    virtual int run();
    void quit();

    Status connect(const std::string& l_password = "");
    Status connect(const sf::Uint16& l_port, const sf::IpAddress& l_ip, const std::string& l_password = "");
    void sendToServer(const std::string& l_text);
private:
    Responses m_responses;
    void unpack(sf::Packet& l_packet);
    bool sendClientDataToServer();
protected:
    ClientData m_client;
    Shared m_shared;
    bool m_running;
    sf::Uint16 m_serverPort;
    sf::IpAddress m_serverIp;
    const std::string m_version;

    Status checkPassword(const std::string& l_password);

    bool sendToServer(sf::Packet& l_packet);

    virtual void onInitialization() = 0;
    virtual void onSuccessfullyConnected() = 0;
    virtual void onErrorWithSendingData() = 0;
    virtual void onErrorWithReceivingData() = 0;
    virtual void onServerClosedConnection() = 0;
    virtual void onServerWrongPassword() = 0;
    virtual void onArgumentsError(const char*) = 0;
    virtual void onUnableToConnect() = 0;
    virtual void onServerIsFull() = 0;
    virtual void onBlockedFromServer() = 0;
    virtual void onError(const std::string& l_text) = 0;
    virtual std::string onServerPasswordNeeded() = 0;


    virtual void onMessageReceived(const std::string&, const ClientType& l_type) = 0;
    virtual void onServerMessageReceived(const std::string&) = 0;
    virtual void onKick() = 0;
    virtual void onPromotion(const std::string& l_text, const bool& l_promotion) = 0;
    virtual void onConnectionNotificationReceived(const std::string&, const Type&) = 0;
    virtual void onServerExit() = 0;

    /// RESPONSES
    void message(sf::Packet& l_packet);
    void serverMessage(sf::Packet& l_packet);
    void kick(sf::Packet& l_packet);
    void promotion(sf::Packet& l_packet);
    void somebodyPromotion(sf::Packet& l_packet);
    void connectionNotification(sf::Packet& l_packet);
    void serverExit(sf::Packet& l_packet);
};

#endif // CLIENT_H
