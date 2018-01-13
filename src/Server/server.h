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
#include <condition_variable>
#include "../Shared/shared.h"

struct ClientServerData{
    ClientServerData() : m_connected(false) {}
    ClientData m_client;
    std::string m_ip;
    bool m_connected;
};

using Clients = std::vector<std::unique_ptr<ClientServerData>>;
using Commands = std::map<std::string, std::function<void()>>;
using CommandsDescriptions = std::map<std::string, std::string>;
using Blocked = std::unordered_set<std::string>;

class Server
{
    ColorChanger m_colorChanger;
public:
    Server();
    ~Server();

    bool processArguments(int& argc, char**& argv);
    void setPassword(const std::string& l_password) { m_password = l_password; }
    void setPort(const sf::Int16& l_port) { m_port = l_port; }

    void printError(const std::string& l_string);
    void printText(const std::string& l_string, const Color& l_color);
    void printServerInfo();


    void run();
private:
    void inputThread();
    sf::Uint16 m_port;
    sf::Uint32 m_max;
    std::string m_password;
    std::string m_version;

    bool m_running;
    std::mutex m_mutex;
    std::condition_variable m_cv;

    Commands m_commands;
    CommandsDescriptions m_commandsDescriptions;
    Clients m_clients;
    std::vector<std::reference_wrapper<std::unique_ptr<ClientServerData>>> m_toRemove;
    Blocked m_blocked;
    sf::TcpListener m_listener;
    sf::SocketSelector m_selector;

    bool sendMessageToAllClientsFrom(std::unique_ptr<ClientServerData>& l_data, const std::string& l_text);
    bool sendMessageToAllClients(const std::string& l_text);
    bool sendMessageToAllClients(sf::Packet& l_packet, std::unique_ptr<ClientServerData>* l_except = nullptr);
    bool sendMessageTo(std::unique_ptr<ClientServerData>& l_data, const std::string& l_text);
    bool sendMessageTo(std::unique_ptr<ClientServerData>& l_data, sf::Packet& l_text);
    bool sendConnectionNotification(const std::string& l_name, const Type& l_type, const Color& l_color, std::unique_ptr<ClientServerData>* l_except = nullptr);
    bool promoteClient(std::unique_ptr<ClientServerData>& l_data, const ClientType& l_type);


    void beginProcessingNewClient(std::unique_ptr<ClientServerData>&& l_socket);
    void endProcessingNewClient(std::unique_ptr<ClientServerData>& l_socket);
    void addNewClient(std::unique_ptr<ClientServerData>& l_socket);
    void remove(std::unique_ptr<ClientServerData>& l_socket);
    void receive();
    void printClientMessage(std::unique_ptr<ClientServerData>& l_data, const std::string& l_message);
    void processRemoving();

    void unpack(const Type& l_type, sf::Packet& l_packet, std::unique_ptr<ClientServerData>& l_data);
/// Commands
    void viewAllClients();
    void viewCommands();
    void changePassword();
    void changeMaxClients();
    void sendMessage();
    void kick();
    void promote();
    void block();
    void unblock();
    void Exit();

    std::string getline();
    bool kickClient(const std::string& _ip);
};

#endif // SERVER_H
