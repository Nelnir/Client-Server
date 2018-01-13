#include "server.h"
#include <algorithm>
#include "../Shared/cxxopts.h"
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
    m_commands.emplace("clients", std::bind(&Server::viewAllClients, this));
    m_commands.emplace("message", std::bind(&Server::sendMessage, this));
    m_commands.emplace("help", std::bind(&Server::viewCommands, this));
    m_commands.emplace("clear", [this]() { system("cls"); });
    m_commands.emplace("info", std::bind(&Server::printServerInfo, this));
    m_commands.emplace("set-password", std::bind(&Server::changePassword, this));
    m_commands.emplace("set-max", std::bind(&Server::changeMaxClients, this));
    m_commands.emplace("exit", std::bind(&Server::Exit, this));
    m_commands.emplace("kick", std::bind(&Server::kick, this));
    m_commands.emplace("block", std::bind(&Server::block, this));
    m_commands.emplace("unblock", std::bind(&Server::unblock, this));
    m_commands.emplace("promote", std::bind(&Server::promote, this));

    m_commandsDescriptions.emplace("clients", "see actually connected clients");
    m_commandsDescriptions.emplace("message", "send message to all connected clients");
    m_commandsDescriptions.emplace("help", "view this message");
    m_commandsDescriptions.emplace("clear", "clear a screen");
    m_commandsDescriptions.emplace("info", "view server info");
    m_commandsDescriptions.emplace("set-password", "changes the server password");
    m_commandsDescriptions.emplace("set-max", "changes the maximum number of clients");
    m_commandsDescriptions.emplace("exit", "close the server");
    m_commandsDescriptions.emplace("kick", "kicks and blocks(only with ip) the client via ip or nickname");
    m_commandsDescriptions.emplace("block", "blocks the client via ip");
    m_commandsDescriptions.emplace("unblock", "unblocks the client via ip");
    m_commandsDescriptions.emplace("promote", "promote the client to administrator(if he owns admin, it will unpromote him)");

    std::cout << sf::IpAddress::Broadcast;
}

Server::~Server()
{
    for(auto& itr : m_clients){
        itr->m_client.m_socket.disconnect();
    }
    m_clients.clear();
    m_selector.clear();
}

void Server::run()
{
    if(m_running) return;
    m_running = true;

    if(m_listener.listen(m_port) != sf::Socket::Done){
        printError("Error when listening on port");
        return;
    }
    m_selector.add(m_listener);

    std::thread input(&Server::inputThread, this);
    std::thread removing(&Server::processRemoving, this);

    while(m_running)
    {
        if(m_selector.wait(sf::milliseconds(500))){
            if(m_selector.isReady(m_listener)){
                auto client = std::make_unique<ClientServerData>();
                if(m_listener.accept(client->m_client.m_socket) == sf::Socket::Done){
                    beginProcessingNewClient(std::move(client));
                }
            }
            receive();
        }
    }

    input.join();
    removing.join();
}

void Server::processRemoving()
{
    while(m_running){
        std::unique_lock<std::mutex> lk(m_mutex);
        m_cv.wait(lk, [this]{return !m_toRemove.empty();});
        while(!m_toRemove.empty()){
            auto& last = m_toRemove.back();
            auto itr2 = std::find(m_clients.begin(), m_clients.end(), last.get());
            if(itr2 != m_clients.end()){
                m_clients.erase(itr2);
                m_toRemove.pop_back();
                continue;
            }

            m_toRemove.pop_back();
        }
    }
}

void Server::beginProcessingNewClient(std::unique_ptr<ClientServerData> && client)
{
    client->m_ip = client->m_client.m_socket.getRemoteAddress().toString();

    sf::Packet packet;
    if(m_blocked.count(client->m_ip)){
        packet << Type::Kick;
        if(!sendMessageTo(client, packet)){
            printError("An error occured when blocking: " + client->m_ip);
            return;
        }
        printText(client->m_ip + " blocked", Color::Red);
        return;
    }

    m_clients.push_back(std::move(client));
    m_selector.add(m_clients.back()->m_client.m_socket);

    if(!m_password.empty()){
        sf::Packet packet;
        packet << Type::ServerPasswordNeeded;
        if(!sendMessageTo(m_clients.back(), packet)){
            printError("Error when sending message to " + client->m_ip);
        }
        return;
    }

    std::thread(&Server::endProcessingNewClient, this, std::ref(m_clients.back())).detach();
}

void Server::endProcessingNewClient(std::unique_ptr<ClientServerData> &client)
{   
    sf::Packet packet;

    {
        std::lock_guard<std::mutex> lk(m_mutex);
        if(std::count_if(m_clients.begin(), m_clients.end(), [](const std::unique_ptr<ClientServerData>& a) { return a->m_connected; }) < static_cast<size_t>(m_max)){
            packet << Type::ServerConnected;
            if(!sendMessageTo(client, packet)){
                return;
            }
            packet.clear();
            if(client->m_client.m_socket.receive(packet) != sf::Socket::Done){
                printError("Error when retrieving: " + client->m_ip + " data");
            } else{
                packet >> client->m_client.m_name >> client->m_client.m_type;
                addNewClient(client);
                return;
            }
        } else{
            packet << Type::ServerIsFull;
            sendMessageTo(client, packet);
            printText(client->m_ip + " rejected", Color::Red);
            remove(client);
        }
    }
    m_cv.notify_one();
}

void Server::receive()
{
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        for(auto& itr : m_clients){
            auto& socket = itr->m_client.m_socket;
            if(m_selector.isReady(socket)){
                sf::Packet packet;
                auto status = socket.receive(packet);
                if(status == sf::Socket::Done){
                    Type type;
                    packet >> type;
                    unpack(type, packet, itr);
                } else if(status == sf::Socket::Disconnected){
                    std::string name = itr->m_client.m_name;
                    printText((name.empty() ? itr->m_ip : name) + " disconnected", Color::Red);

                    /// if name isnt empty, it means that client is on server (not waiting clients)
                    if(!name.empty()){
                        sendConnectionNotification(name, Type::Disconnection, Color::Red, &itr);
                    }

                    remove(itr);
                }
                break;
            }
        }
    }
    m_cv.notify_one();
}

void Server::inputThread()
{
    std::string input;
    while(m_running){
        std::cin >> input;
        auto itr = m_commands.find(input);
        if(itr == m_commands.end()){
            printError("Unknown command, type help to see available commands");
            continue;
        }
        itr->second();
    }
}

void Server::printClientMessage(std::unique_ptr<ClientServerData> &l_data, const std::string &l_message)
{
    Color color = Color::Default;
    if(l_data->m_client.m_type >= ClientType::Administrator){
        color = Color::Yellow;
    }
    printText(l_data->m_client.m_name + ": " + l_message, color);
}

void Server::unpack(const Type &l_type, sf::Packet &l_packet, std::unique_ptr<ClientServerData>& l_user)
{
    switch(l_type)
    {
    case Type::Message:{
        std::string text;
        l_packet >> text;
        printClientMessage(l_user, text);
        sendMessageToAllClientsFrom(l_user, text);
        break;
    }
    case Type::Password:{
        std::string text;
        l_packet >> text;

        sf::Packet packet;
        if(text == m_password){
            std::thread(&Server::endProcessingNewClient, this, std::ref(l_user)).detach();
           // endProcessingNewClient(l_user);
        } else{
            packet << Type::ServerPasswordNeeded;
            if(!sendMessageTo(l_user, packet)){
                printError("Error when sending message to: " + l_user->m_ip);
            }
        }
        break;
    }
    }
}

void Server::addNewClient(std::unique_ptr<ClientServerData>& l_client)
{
    printText(l_client->m_client.m_name + '(' + l_client->m_ip + ") connected", Color::Blue);
    l_client->m_client.m_socket.setBlocking(false);
    l_client->m_connected = true;

    sendConnectionNotification(l_client->m_client.m_name, Type::Connection, Color::Green, &l_client);
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
            return false;
        }
    }
    return true;
}

bool Server::sendConnectionNotification(const std::string &l_name, const Type& l_type, const Color& l_color, std::unique_ptr<ClientServerData>* l_except)
{
    sf::Packet packet;
    packet << Type::Connection << l_name << l_type << l_color;
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
        printError("Error when sending message to client: " + l_data->m_ip);
        return false;
    }
    return true;
}

void Server::printServerInfo()
{
    m_colorChanger.setConsoleTextColor(Color::White);
    std::cout << "Server public address: " << sf::IpAddress::getPublicAddress() << std::endl
        << "local address: " << sf::IpAddress::getLocalAddress() << std::endl
        << "port: " << m_port << std::endl
        << "password: " << m_password << std::endl;

    std::cout << "maximum clients: "; if(m_max != sf::Uint32(-1)) std::cout << m_max; std::cout << std::endl;
    std::cout << "version: " << m_version << std::endl;
    m_colorChanger.setConsoleTextColor(Color::Default);
}

void Server::printError(const std::string &l_string)
{
    Color tmp = m_colorChanger.m_color;
    m_colorChanger.setConsoleTextColor(Color::Red);
    std::cerr << l_string << std::endl;
    m_colorChanger.setConsoleTextColor(tmp);
}

void Server::printText(const std::string &l_string, const Color& l_color)
{
    Color tmp = m_colorChanger.m_color;
    m_colorChanger.setConsoleTextColor(l_color);
    std::cerr << l_string << std::endl;
    m_colorChanger.setConsoleTextColor(tmp);
}


void Server::viewAllClients()
{
    std::lock_guard<std::mutex> lk(m_mutex);
    size_t connected = 0, waiting = 0;
    for(auto& itr : m_clients){
        if(itr->m_connected) ++connected;
        else ++waiting;
    }

    m_colorChanger.setConsoleTextColor(Color::White);
    std::cout << "Connected clients: " << connected << " / " << m_max << std::endl;
    m_colorChanger.setConsoleTextColor(Color::Blue);
    for(auto& itr : m_clients){
        if(!itr->m_connected) continue;
        std::cout << itr->m_client.m_name << " - " << itr->m_ip << (itr->m_client.m_type == ClientType::Administrator ? "[ADMIN]" : "") << std::endl;
    }
    m_colorChanger.setConsoleTextColor(Color::White);
    std::cout << "Waiting clients: " << waiting << std::endl;
    m_colorChanger.setConsoleTextColor(Color::Blue);
    for(auto& itr : m_clients){
        if(itr->m_connected) continue;
        std::cout << itr->m_ip << std::endl;
    }
    m_colorChanger.setConsoleTextColor(Color::Default);
}

void Server::viewCommands()
{
    for(auto& itr : m_commands){
        m_colorChanger.setConsoleTextColor(Color::Yellow);
        std::cout << "    " << itr.first;
        m_colorChanger.setConsoleTextColor(Color::Default);

        auto itr2 = m_commandsDescriptions.find(itr.first);
        if(itr2 != m_commandsDescriptions.end()){
            std::cout << " - " << itr2->second;
        }
        std::cout << std::endl;
    }
}

bool Server::promoteClient(std::unique_ptr<ClientServerData> &l_data, const ClientType &l_type)
{
    sf::Packet packet;
    packet << Type::Promotion << l_type;
    if(sendMessageTo(l_data, packet)){
        l_data->m_client.m_type = l_type;
        packet.clear();
        packet << Type::Update << l_type << l_data->m_client.m_name;
        return sendMessageToAllClients(packet, &l_data);
    }
    return false;
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
            m_max = result["max"].as<sf::Uint32>();
        }
        if(result.count("password")){
            m_password = result["password"].as<std::string>();
        }
    }
    catch(std::exception& ex){
       printError(ex.what());
       return false;
    }
    return true;
}

void Server::Exit()
{
    m_running = false;
    m_listener.close();
}

void Server::sendMessage()
{
    sendMessageToAllClients(getline());
}

void Server::changePassword()
{
    setPassword(getline());
}

void Server::changeMaxClients()
{
    m_max = std::stoi(getline());
}

void Server::kick()
{
    if(kickClient(getline())){
        printText("Client have been kicked", Color::Green);
    } else{
        printError("An error occured");
    }
}

void Server::block()
{
    std::string s = getline();
    if(m_blocked.emplace(s).second){
        printText(s + " have been blocked", Color::Green);
    } else {
        printError(s + " is already blocked");
    }
}

void Server::unblock()
{
    std::string s = getline();
    auto itr = m_blocked.find(s);
    if(itr == m_blocked.end()){
        printText(s + " is not blocked", Color::Red);
    } else{
        m_blocked.erase(itr);
        printText(s + " have been unblocked", Color::Green);
    }
}

void Server::promote()
{
    std::string s = getline();
    for(auto& itr : m_clients){
        if(itr->m_ip != s && itr->m_client.m_name != s){
            continue;
        }
        ClientType type = ClientType::Normie;
        if(itr->m_client.m_type == ClientType::Normie){
            type = ClientType::Administrator;
        }

        if(promoteClient(itr, type)){
            if(type == ClientType::Administrator){
                printText("Client have been promoted", Color::Green);
            } else{
                printText("Client have been degraded", Color::Red);
            }
        } else{
            printError("An error occured");
        }
        return;
    }
    printError("Unable to find client");
}

void Server::remove(std::unique_ptr<ClientServerData> &l_socket)
{
    m_selector.remove(l_socket->m_client.m_socket);
    m_toRemove.push_back(l_socket);
}

bool Server::kickClient(const std::string &l_text)
{
    std::lock_guard<std::mutex> lk(m_mutex);

    for(auto& itr : m_clients){
        if(itr->m_ip == l_text){
            m_blocked.emplace(l_text);
        } else if(itr->m_client.m_name != l_text){
            continue;
        }

        sf::Packet packet;
        packet << Type::Kick;
        std::string name = itr->m_client.m_name;
        sendMessageTo(itr, packet);
        sendConnectionNotification(name, Type::Kick, Color::Red, &itr);
        remove(itr);
        m_cv.notify_one();
        return true;
    }
    return false;
}

std::string Server::getline()
{
    std::cout << "    >>";
    std::string pass;
    m_colorChanger.setConsoleTextColor(Color::White);
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::getline(std::cin, pass);
    m_colorChanger.setConsoleTextColor(Color::Default);
    return pass;
}
