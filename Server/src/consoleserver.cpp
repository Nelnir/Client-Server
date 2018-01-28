#include "consoleserver.h"

ConsoleServer::ConsoleServer()
{
    m_commands.emplace("clients", std::bind(&ConsoleServer::viewAllClients, this));
    m_commands.emplace("message", std::bind(&ConsoleServer::sendMessage, this));
    m_commands.emplace("help", std::bind(&ConsoleServer::viewAllCommands, this));
    m_commands.emplace("clear", [this]() { system("cls"); });
    m_commands.emplace("info", std::bind(&ConsoleServer::printServerInfo, this));
    m_commands.emplace("set-password", std::bind(&ConsoleServer::changePassword, this));
    m_commands.emplace("set-max", std::bind(&ConsoleServer::changeMaxClients, this));
    m_commands.emplace("exit", std::bind(&ConsoleServer::quit, this));
    m_commands.emplace("kick", std::bind(&ConsoleServer::kick, this));
    m_commands.emplace("block", std::bind(&ConsoleServer::block, this));
    m_commands.emplace("unblock", std::bind(&ConsoleServer::unblock, this));
    m_commands.emplace("promote", std::bind(&ConsoleServer::promote, this));
    m_commands.emplace("promote-help", std::bind(&ConsoleServer::helpPromote, this));

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
    m_commandsDescriptions.emplace("promote", "promote the client via ip or nickname");
    m_commandsDescriptions.emplace("promote-help", "view help message");
}

ConsoleServer::~ConsoleServer()
{

}

int ConsoleServer::run()
{
    m_running = true;
    printServerInfo();
    std::thread(&ConsoleServer::inputThread, this).detach();
    return Server::run();
}

void ConsoleServer::inputThread()
{
    std::string input;
    while(m_running){
        std::cin >> input;
        auto itr = m_commands.find(input);
        if(itr == m_commands.end()){
            printText("Unknown command, type help to see available commands", Color::Red);
            continue;
        }
        itr->second();
    }
}

void ConsoleServer::printServerInfo()
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

void ConsoleServer::printError(const std::string &l_string)
{
    Color tmp = m_colorChanger.m_color;
    m_colorChanger.setConsoleTextColor(Color::Red);
    std::cerr << l_string << std::endl;
    m_colorChanger.setConsoleTextColor(tmp);
}

void ConsoleServer::printText(const std::string &l_string, const Color& l_color)
{
    Color tmp = m_colorChanger.m_color;
    m_colorChanger.setConsoleTextColor(l_color);
    std::cout << l_string << std::endl;
    m_colorChanger.setConsoleTextColor(tmp);
}

void ConsoleServer::onClientBlocked(std::unique_ptr<ClientServerData> &l_client)
{
    printText(l_client->m_ip + " blocked", Color::Red);
}

void ConsoleServer::onClientRejected(std::unique_ptr<ClientServerData> &l_client)
{
    printText(l_client->m_ip + " rejected", Color::Red);
}

void ConsoleServer::onClientConnected(std::unique_ptr<ClientServerData> &l_client)
{
    printText(l_client->m_client.m_name + '(' + l_client->m_ip +  ") connected", Color::Green);
}

void ConsoleServer::onClientDisconnected(std::unique_ptr<ClientServerData> &l_client)
{
    std::string name = l_client->m_client.m_name;
    printText((name.empty() ? l_client->m_ip : name) + " disconnected", Color::Red);
}

void ConsoleServer::onClientPromoted(std::unique_ptr<ClientServerData>& l_client, const bool& l_promoted)
{
    if(l_promoted){
        printText(l_client->m_client.m_name + " has been promoted", Color::Green);
    } else{
        printText(l_client->m_client.m_name + " has been degraded", Color::Red);
    }
}

void ConsoleServer::onErrorWithReceivingData(std::unique_ptr<ClientServerData> &l_client)
{
    printError("Error when retrieving data from: " + l_client->m_ip);
}

void ConsoleServer::onErrorWithSendingData(std::unique_ptr<ClientServerData> &l_client)
{
    printError("Error when sending data to: " + l_client->m_ip);
}

void ConsoleServer::onArgumentsError(const char *l_what)
{
    printError(l_what);
}

void ConsoleServer::onClientMessageReceived(std::unique_ptr<ClientServerData> &l_client, const std::string &l_text)
{
    std::string name = l_client->m_client.m_name;
    if(l_client->m_client.m_type == ClientType::Administrator){
        name += "[ADMIN]";
    }
    name += ": ";
    printText(name + l_text, Color::Default);
}

void ConsoleServer::error(const std::string &l_text)
{
    printError(l_text);
}

void ConsoleServer::viewAllClients()
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
        std::cout << itr->m_client.m_name << '[' << std::to_string(static_cast<int>(itr->m_client.m_type)) << ']' << " - " << itr->m_ip << std::endl;
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

void ConsoleServer::viewAllCommands()
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

std::string ConsoleServer::getline()
{
    std::cout << "    >>";
    std::string pass;
    m_colorChanger.setConsoleTextColor(Color::White);
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::getline(std::cin, pass);
    m_colorChanger.setConsoleTextColor(Color::Default);
    return pass;
}

void ConsoleServer::printClientMessage(std::unique_ptr<ClientServerData> &l_data, const std::string &l_message)
{
    std::string name = l_data->m_client.m_name;
    if(l_data->m_client.m_type >= ClientType::Administrator){
        name += "[ADMIN]";
    }
    printText(name + ": " + l_message, Color::Default);
}

void ConsoleServer::sendMessage()
{
    sendMessageToAllClients(getline());
}

void ConsoleServer::changePassword()
{
    setPassword(getline());
}

void ConsoleServer::changeMaxClients()
{
    try{
        setMaxNumberOfClients(std::stoi(getline()));
    }
    catch(const std::logic_error& ex){
        printError(ex.what());
        return;
    }
}

void ConsoleServer::kick()
{
    if(Server::kick(getline(), true)){
        printText("Client have been kicked", Color::Green);
    } else{
        printError("An error occured");
    }
}

void ConsoleServer::block()
{
    std::string s = getline();
    if(m_blocked.emplace(s).second){
        printText(s + " have been blocked", Color::Green);
    } else {
        printError(s + " is already blocked");
    }
}

void ConsoleServer::unblock()
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

void ConsoleServer::promote()
{
    std::string name = getline();
    size_t pos = name.find_last_of(':');
    if(pos != std::string::npos){
        try{
            ClientType type = static_cast<ClientType>(stoi(name.substr(pos + 1, std::string::npos)));
            name = name.substr(0, pos);
            if(Server::promote(name, type)){
                return;
            }
        }
        catch(const std::logic_error& ex){
            printError(ex.what());
            return;
        }
    }
    printText("An error occured", Color::Red);
}

void ConsoleServer::helpPromote()
{
    printText("Available types: ", Color::Default);
    for(auto& itr : m_shared.m_names){
        printText(std::to_string(static_cast<int>(itr.first)) + " - " + itr.second, Color::White);
    }
    printText("Usage: %ip/nickname%:number", Color::Yellow);
}
