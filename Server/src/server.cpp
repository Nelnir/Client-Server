#include "server.h"
#include <algorithm>
#include "../../Shared/cxxopts.h"
#include <utility>
#include <thread>
#include <chrono>

Server::Server() :
    m_port(0),
    m_max(-1),
    m_password(""),
    m_version("1.0"),
    m_running(false)
{

}

Server::~Server()
{
    for(auto& itr : m_clients){
        itr->m_client.m_socket.disconnect();
    }
    m_clients.clear();
    m_blocked.clear();
    m_selector.clear();
    m_listener.close();
}

int Server::run()
{
    m_running = true;
    if(m_listener.listen(m_port) != sf::Socket::Done){
        error("Error when listening on port: " + m_port);
        return -1;
    }
    m_selector.add(m_listener);
    while(m_running)
    {
        if(m_selector.wait(sf::milliseconds(50))){
            if(m_selector.isReady(m_listener)){
                auto client = std::make_unique<ClientServerData>();
                if(m_listener.accept(client->m_client.m_socket) == sf::Socket::Done){
                    client->m_ip = client->m_client.m_socket.getRemoteAddress().toString();
                    if(!isBlocked(client->m_ip)){
                        processNewClient(std::move(client));
                    } else{
                        sf::Packet packet;
                        packet << Type::Kick;
                        if(!sendMessageTo(client, packet)){
                            onErrorWithSendingData(client);
                        }
                        onClientBlocked(client);
                    }
                }
            }
            std::lock_guard<std::mutex> lk(m_mutex);
            auto itr = std::begin(m_clients);
            while(itr != std::end(m_clients)){
                auto& socket = (*itr)->m_client.m_socket;
                if(m_selector.isReady(socket)){
                    sf::Packet packet;
                    auto status = socket.receive(packet);
                    if(status == sf::Socket::Done){
                        onClientPacketReceived(*itr, packet);
                    } else if(status == sf::Socket::Disconnected){
                        if((*itr)->m_connected){
                            onClientDisconnected(*itr);
                            sendConnectionNotification((*itr)->m_client.m_name, Type::Disconnection, &*itr);
                        }
                        m_selector.remove(socket);
                        itr = m_clients.erase(itr);
                        continue;
                    }else if (status == sf::Socket::Error){
                        onErrorWithReceivingData(*itr);
                    }
                }
                ++itr;
            }
        }
    }
    return 0;
}

void Server::processNewClient(std::unique_ptr<ClientServerData> && client)
{
    m_clients.push_back(std::move(client));
    m_selector.add(m_clients.back()->m_client.m_socket);

    if(!m_password.empty()){
        sf::Packet packet;
        packet << Type::ServerPasswordNeeded;
        sendMessageTo(m_clients.back(), packet);
        return;
    }

    std::thread(&Server::prepareNewClient, this, std::ref(m_clients.back())).detach();
}

bool Server::isBlocked(const std::string &l_ip)
{
    return m_blocked.count(l_ip) ? true : false;
}

void Server::prepareNewClient(std::unique_ptr<ClientServerData> &client)
{
    sf::Packet packet;
    std::lock_guard<std::mutex> lk(m_mutex);
    if(static_cast<sf::Uint32>(std::count_if(m_clients.begin(), m_clients.end(), [](const std::unique_ptr<ClientServerData>& a) { return a->m_connected; })) < m_max){
        packet << Type::ServerConnected;
        sendMessageTo(client, packet);
        packet.clear();
        auto status = client->m_client.m_socket.receive(packet);
        if(status != sf::Socket::Done){
            onErrorWithReceivingData(client);
        } else{
            packet >> client->m_client.m_name >> client->m_client.m_type;
            finishNewClient(client);
        }
    } else{
        packet << Type::ServerIsFull;
        sendMessageTo(client, packet);
        onClientRejected(client);
        m_selector.remove(client->m_client.m_socket);
        m_clients.erase(std::find(std::begin(m_clients), std::end(m_clients), client));
    }
}

void Server::finishNewClient(std::unique_ptr<ClientServerData>& l_client)
{
    l_client->m_client.m_socket.setBlocking(false);
    l_client->m_connected = true;
    onClientConnected(l_client);
    sendConnectionNotification(l_client->m_client.m_name, Type::Connection, &l_client);
}

bool Server::sendMessageToAllClientsFrom(std::unique_ptr<ClientServerData>& l_data, const std::string &l_text)
{
    sf::Packet packet;
    packet << Type::Message;
    packet << l_data->m_client.m_type << l_data->m_client.m_name << l_text;
    return sendMessageToAllClients(packet, &l_data);
}

bool Server::sendMessageToAllClients(const std::string &l_text)
{
    sf::Packet packet;
    packet << Type::ServerMessage << l_text;
    return sendMessageToAllClients(packet);
}

bool Server::sendMessageToAllClients(sf::Packet &l_packet, std::unique_ptr<ClientServerData>* l_except)
{
    for(auto& itr : m_clients){
        if(&itr == l_except || !itr->m_connected){
            continue;
        }
        if(!sendMessageTo(itr, l_packet)){
            onErrorWithSendingData(itr);
        }
    }
    return true;
}

bool Server::sendConnectionNotification(const std::string &l_name, const Type& l_type, std::unique_ptr<ClientServerData>* l_except)
{
    sf::Packet packet;
    packet << Type::Connection << l_name << l_type;
    return sendMessageToAllClients(packet, l_except);
}

bool Server::sendMessageTo(std::unique_ptr<ClientServerData>& l_data, const std::string &l_text)
{
    sf::Packet packet;
    packet << Type::ServerMessage << l_text;
    return sendMessageTo(l_data, packet);
}

bool Server::sendMessageTo(std::unique_ptr<ClientServerData>& l_data, sf::Packet& l_packet)
{
    if(l_data->m_client.m_socket.send(l_packet) == sf::Socket::Error){
        onErrorWithSendingData(l_data);
        return false;
    }
    return true;
}

bool Server::processArguments(int& argc, char **&argv)
{
    if(argc == 1){
        std::cout << "Pass -h, --help to view help message" << std::endl;
        return false;
    }
    cxxopts::Options options("Server", "Just educational application");
    options.add_options()
        ("h,help", "View this message")
        ("password", "Set server password", cxxopts::value<std::string>())
        ("port", "Set port number", cxxopts::value<sf::Uint16>())
        ("max", "Set maximum clients (default is unlimited)", cxxopts::value<sf::Uint32>())
    ;
    try
    {
        auto result = options.parse(argc, argv);
        if(result.count("help")){
            std::cout << options.help();
            return false;
        }
        if(result.count("port")){
            setPort(result["port"].as<sf::Uint16>());
        }
        if(result.count("max")){
            setMaxNumberOfClients(result["max"].as<sf::Uint32>());
        }
        if(result.count("password")){
            m_password = result["password"].as<std::string>();
        }
    }
    catch(std::exception& ex){
       onArgumentsError(ex.what());
       return false;
    }
    return true;
}

bool Server::promote(const std::string& l_ip, const ClientType& l_type)
{
    std::lock_guard<std::mutex> lk(m_mutex);
    for(auto& itr : m_clients){
        if(sf::IpAddress(itr->m_ip) != l_ip && itr->m_client.m_name != l_ip){
            continue;
        }

        if(!promoteClient(itr, l_type)){
            onErrorWithSendingData(itr);
        }
        return true;
    }
    return false;
}

bool Server::promoteClient(std::unique_ptr<ClientServerData> &l_data, const ClientType &l_type)
{
    sf::Packet packet;
    packet << Type::Promotion << l_type;
    if(sendMessageTo(l_data, packet)){
        bool promoted = false;
        if(l_data->m_client.m_type < l_type){
            promoted = true;
        }
        l_data->m_client.m_type = l_type;
        onClientPromoted(l_data, promoted);
        packet.clear();
        packet << Type::SomebodyPromotion << l_type << l_data->m_client.m_name << promoted;
        sendMessageToAllClients(packet, &l_data);
        return true;
    }
    return false;
}

bool Server::kick(const std::string &l_ip, const bool& l_block)
{
    std::lock_guard<std::mutex> lk(m_mutex);

    auto itr = std::begin(m_clients);
    while(itr != std::end(m_clients)){
        if(!(*itr)->m_connected){
            ++itr;
            continue;
        }
        if((*itr)->m_ip == l_ip){
            if(l_block) m_blocked.emplace(l_ip);
        } else if((*itr)->m_client.m_name != l_ip){
            ++itr;
            continue;
        }

        sf::Packet packet;
        packet << Type::Kick;
        std::string name = (*itr)->m_client.m_name;
        sendMessageTo(*itr, packet);
        sendConnectionNotification(name, Type::Kick, &*itr);
        m_selector.remove((*itr)->m_client.m_socket);
        m_clients.erase(itr);
        ++itr;
        return true;
    }
    return false;
}

void Server::onClientPacketReceived(std::unique_ptr<ClientServerData> &l_client, sf::Packet &l_packet)
{
    Type type;
    l_packet >> type;
    switch(type)
    {
    case Type::Message:{
        std::string text;
        l_packet >> text;
        onClientMessageReceived(l_client, text);
        sendMessageToAllClientsFrom(l_client, text);
        break;
        }
    case Type::Password:{
        std::string text;
        l_packet >> text;
        if(text == m_password){
            std::thread(&Server::prepareNewClient, this, std::ref(l_client)).detach();
        } else{
            sf::Packet packet;
            packet << Type::ServerPasswordNeeded;
            sendMessageTo(l_client, packet);
        }
        break;
        }
    }
}

void Server::quit()
{
    sf::Packet packet;
    packet << Type::ServerExit;
    sendMessageToAllClients(packet);
    m_running = false;
}
