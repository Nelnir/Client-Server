#include "client.h"
#include "../../Shared/cxxopts.h"
#include <thread>

Client::Client() :
    m_serverIp(""),
    m_serverPort(0),
    m_version("1.0"),
    m_running(false)
{
    m_responses.emplace(Type::Message, std::bind(&Client::message, this, std::placeholders::_1));
    m_responses.emplace(Type::ServerMessage, std::bind(&Client::serverMessage, this, std::placeholders::_1));
    m_responses.emplace(Type::Kick, std::bind(&Client::kick, this, std::placeholders::_1));
    m_responses.emplace(Type::Connection, std::bind(&Client::connectionNotification, this, std::placeholders::_1));
    m_responses.emplace(Type::Promotion, std::bind(&Client::promotion, this, std::placeholders::_1));
    m_responses.emplace(Type::SomebodyPromotion, std::bind(&Client::somebodyPromotion, this, std::placeholders::_1));
    m_responses.emplace(Type::ServerExit, std::bind(&Client::serverExit, this, std::placeholders::_1));
}

Client::~Client()
{

}

Status Client::checkPassword(const std::string &l_password)
{
    sf::Packet packet;
    packet << l_password;
    if(m_client.m_socket.send(packet) != sf::Socket::Done){
        onErrorWithSendingData();
    }
    packet.clear();
    if(m_client.m_socket.receive(packet) != sf::Socket::Done){
        onErrorWithReceivingData();
    }
    Type type;
    packet >> type;
    if(type == Type::ServerConnected){
        return Status::Connected;
    } else if(type == Type::ServerIsFull){
        return Status::ServerIsFull;
    }
    return Status::WrongPassword;
}

Status Client::connect(const std::string& l_password)
{
    if(m_client.m_socket.connect(m_serverIp, m_serverPort, sf::seconds(2)) == sf::Socket::Done){
        sf::Packet packet;
        if(m_client.m_socket.receive(packet) == sf::Socket::Done){
            Type type;
            packet >> type;
            switch(type)
            {
                case Type::ServerConnected:      return Status::Connected;
                case Type::ServerIsFull:         return Status::ServerIsFull;
                case Type::Kick:                 return Status::Blocked;
                case Type::ServerPasswordNeeded: return checkPassword(l_password);
            }
        }
    }
    return Status::UnableToConnect;
}

bool Client::establishConnection()
{
    onInitialization();
    Status status = connect();
    if(status == Status::WrongPassword){
        do{
            std::string password = onServerPasswordNeeded();
            status = connect(password);
            if(status == Status::WrongPassword){
                onServerWrongPassword();
            }
        }while(status == Status::WrongPassword);
    }

    if(status == Status::Connected){
        if(sendClientDataToServer()){
            onSuccessfullyConnected();
            return true;
        }
    } else if(status == Status::UnableToConnect){
        onUnableToConnect();
    } else if(status == Status::ServerIsFull){
        onServerIsFull();
    } else if(status == Status::Blocked){
        onBlockedFromServer();
    }
    return false;
}

Status Client::connect(const sf::Uint16 &l_port, const sf::IpAddress &l_ip, const std::string &l_password)
{
    setPort(l_port);
    setIp(l_ip);
    return connect(l_password);
}

bool Client::sendClientDataToServer()
{
    sf::Packet packet;
    packet << m_client.m_name << m_client.m_type;
    if(m_client.m_socket.send(packet) != sf::Socket::Done){
        onErrorWithSendingData();
        return false;
    }
    return true;
}

bool Client::processArguments(int &argc, char **&argv)
{
    if(argc == 1){
        return true;
    }
    cxxopts::Options options("Client", "version: " + m_version);
    options.add_options()
        ("h,help", "View this message")
    ;
    try
    {
        auto result = options.parse(argc, argv);
        if(result.count("help")){
            std::cout << options.help();
            return false;
        }
    }
    catch(std::exception& ex){
       onArgumentsError(ex.what());
       return false;
    }

    return true;
}

int Client::run()
{
    m_running = true;
    while(m_running){
        sf::Packet packet;
        auto status = m_client.m_socket.receive(packet);
        if(status == sf::Socket::Done){
            unpack(packet);
        } else if(status == sf::Socket::Disconnected){
            onServerClosedConnection();
            quit();
        }
    }
    return 0;
}

void Client::quit()
{
    m_client.m_socket.disconnect();
    m_running = false;
}

void Client::message(sf::Packet &l_packet)
{
    std::string username, message;
    ClientType type;
    l_packet >> type >> username >> message;
    if(type == ClientType::Administrator){
        username += "[ADMIN]";
    }
    username += ": ";
    onMessageReceived(username, type);
}

void Client::serverMessage(sf::Packet &l_packet)
{
    std::string message;
    l_packet >> message;
    onServerMessageReceived("[SERVER]: " + message);
}

void Client::kick(sf::Packet &l_packet)
{
    onKick();
    m_running = false;
}

void Client::connectionNotification(sf::Packet &l_packet)
{
    std::string name;
    Type type2;
    l_packet >> name >> type2;
    if(type2 == Type::Connection) name += " joined";
    else if(type2 == Type::Kick) name += " have been kicked";
    else if(type2 == Type::Disconnection) name += " disconnected";
    else name += " [unknown reason]";
    onConnectionNotificationReceived(name, type2);
}

void Client::promotion(sf::Packet &l_packet)
{
    ClientType type;
    l_packet >> type;
    if(m_client.m_type < type){
        onPromotion("You have been promoted to: " + m_shared.getNameFor(type), true);
    } else{
        onPromotion("You have been degraded to: " + m_shared.getNameFor(type), false);
    }
    m_client.m_type = type;
}

void Client::somebodyPromotion(sf::Packet &l_packet)
{
    ClientType type;
    std::string name;
    bool promoted;
    l_packet >> type >> name >> promoted;
    if(promoted){
        name += " has been promoted to: " + m_shared.getNameFor(type);
    } else{
        name += " has been degraded to: " + m_shared.getNameFor(type);
    }
    onPromotion(name, promoted);
}

void Client::serverExit(sf::Packet &l_packet)
{
    onServerExit();
    m_running = false;
}

void Client::unpack(sf::Packet &packet)
{
    Type type;
    packet >> type;
    auto itr = m_responses.find(type);
    if(itr == m_responses.end()){
        onError("Unknown message received from server");
    }
    itr->second(packet);
}

bool Client::sendToServer(sf::Packet &l_packet)
{
    return m_client.m_socket.send(l_packet) == sf::Socket::Done;
}

void Client::sendToServer(const std::string &l_text)
{
    sf::Packet packet;
    packet << Type::Message << l_text;
    if(!sendToServer(packet)){
        onErrorWithSendingData();
    }
}
