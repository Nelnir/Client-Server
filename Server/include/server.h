#ifndef SERVER_H
#define SERVER_H

#include <SFML/Network.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <functional>
#include <unordered_set>
#include "../../Shared/shared.h"

struct ClientServerData{
    ClientServerData() : m_connected(false) {}
    ClientData m_client;
    std::string m_ip;
    bool m_connected;
};

using Clients = std::vector<std::unique_ptr<ClientServerData>>;
using Blocked = std::unordered_set<std::string>;

class Server
{
public:
    Server();
    ~Server();

    bool processArguments(int& argc, char**& argv);

    /// SETTERS
    void setPassword(const std::string& l_password) { m_password = l_password; }
    void setPort(const sf::Uint16& l_port) { m_port = l_port; }
    void setMaxNumberOfClients(const sf::Uint32& l_max) { m_max = l_max; }

    /// GETTERS
    std::string getPassword() { return m_password; }
    sf::Uint16 getPort() { return m_port; }
    sf::Uint32 getMaxNumberOfClients() { return m_max; }

    /// UTILITIES
    bool block(const std::string& l_ip);
    bool unblock(const std::string& l_ip);
    bool isBlocked(const std::string& l_ip);
    bool kick(const std::string& l_ip, const bool& l_block = false);
    bool promote(const std::string& l_ip, const ClientType& l_type);
    bool promoteClient(std::unique_ptr<ClientServerData> &l_data, const ClientType &l_type);

    /// MAIN FUNCTIONS
    virtual int run();
    void quit();
private:
    sf::TcpListener m_listener;
    sf::SocketSelector m_selector;


    void processNewClient(std::unique_ptr<ClientServerData> && l_socket);
    void finishNewClient(std::unique_ptr<ClientServerData>& l_socket);
    void prepareNewClient(std::unique_ptr<ClientServerData>& l_socket);

    void onClientPacketReceived(std::unique_ptr<ClientServerData>& l_client, sf::Packet& l_packet);
protected:
    Shared m_shared;
    std::mutex m_mutex;
    Clients m_clients;
    Blocked m_blocked;
    sf::Uint16 m_port;
    sf::Uint32 m_max;
    std::string m_password;
    std::string m_version;
    bool m_running;

    bool sendMessageToAllClientsFrom(std::unique_ptr<ClientServerData>& l_client, const std::string& l_text);
    bool sendMessageToAllClients(const std::string& l_text);
    bool sendMessageToAllClients(sf::Packet& l_packet, std::unique_ptr<ClientServerData>* l_except = nullptr);
    bool sendMessageTo(std::unique_ptr<ClientServerData>& l_data, const std::string& l_text);
    bool sendMessageTo(std::unique_ptr<ClientServerData>& l_data, sf::Packet& l_text);
    bool sendConnectionNotification(const std::string& l_name, const Type& l_type, std::unique_ptr<ClientServerData>* l_except = nullptr);

    virtual void onClientBlocked(std::unique_ptr<ClientServerData>& l_client) = 0;
    virtual void onClientRejected(std::unique_ptr<ClientServerData>& l_client) = 0;
    virtual void onClientConnected(std::unique_ptr<ClientServerData>& l_client) = 0;
    virtual void onClientDisconnected(std::unique_ptr<ClientServerData>& l_client) = 0;
    virtual void onClientMessageReceived(std::unique_ptr<ClientServerData>& l_client, const std::string& l_text) = 0;
    virtual void onClientPromoted(std::unique_ptr<ClientServerData>& l_client, const bool& l_promoted) = 0;
    virtual void onErrorWithReceivingData(std::unique_ptr<ClientServerData>& l_client) = 0;
    virtual void onErrorWithSendingData(std::unique_ptr<ClientServerData>& l_client) = 0;
    virtual void onArgumentsError(const char*) = 0;
    virtual void error(const std::string& l_error) = 0;
};

#endif // SERVER_H
