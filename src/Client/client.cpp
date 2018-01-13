#include "client.h"
#include "../Shared/cxxopts.h"
#include <thread>

Client::Client() :
    m_serverIp(""),
    m_serverPort(0),
    m_inputData(true),
    m_version("1.0"),
    m_running(false)
{

}

Client::~Client()
{

}

void Client::inputData()
{
    if(!m_inputData){
        return;
    }
    m_colorChanger.setConsoleTextColor(Color::White);
    std::cout << "Username: ";    std::cin >> m_client.m_name;
    std::cout << "Server ip: ";   std::cin >> m_serverIp;
    std::cout << "Server port: "; std::cin >> m_serverPort;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    m_colorChanger.setConsoleTextColor(Color::Default);
}

Status Client::connect()
{
    if(m_client.m_socket.connect(sf::IpAddress(m_serverIp), m_serverPort, sf::seconds(2)) == sf::Socket::Done){
        sf::Packet packet;
        if(m_client.m_socket.receive(packet) == sf::Socket::Done){
            Type type;
            packet >> type;
            switch(type)
            {
                case Type::ServerConnected:{
                    if(!sendClientDataToServer()){
                        return Status::ErrorWhenSendingData;
                    }
                    return Status::Success;
                    break;
                }
                case Type::ServerIsFull:{
                    return Status::ServerIsFull;
                    break;
                }
                case Type::ServerPasswordNeeded:{
                    Status status = inputServerPassword();
                    if(status == Status::Success){
                        if(!sendClientDataToServer()){
                            return Status::ErrorWhenSendingData;
                        }
                    }
                    return status;
                    break;
                }
            case Type::Kick:{
                    return Status::Blocked;
                    break;
                }
            }
        } else{
            return Status::ErrorWhenRetrievingData;
        }
    } else{
        return Status::UnableToConnect;
    }
    return Status::Exit;
}

void Client::successfullyConnected()
{
    printText("Successfully connected", Color::Green);
}

Status Client::checkServerPassword(const std::string &l_pass)
{
    sf::Packet packet;
    packet << Type::Password;
    packet << l_pass;
    if(m_client.m_socket.send(packet) != sf::Socket::Done){
        return Status::ErrorWhenSendingData;
    }

    packet.clear();

    if(m_client.m_socket.receive(packet) != sf::Socket::Done){
        return Status::ErrorWhenRetrievingData;
    }
    Type type;
    packet >> type;
    if(type == Type::ServerConnected){
        return Status::Success;
    } else if(type == Type::ServerIsFull){
        return Status::ServerIsFull;
    }
    return Status::Wrong;
}

Status Client::inputServerPassword()
{
    m_colorChanger.setConsoleTextColor(Color::White);
    std::string pass;
    Status passed;
    do{
        std::cout << "Password: ";
        std::cin >> pass;
        passed = checkServerPassword(pass);
        if(passed == Status::Wrong && pass != "exit" && passed != Status::ServerIsFull){
            printError("Wrong password");
        }
    }while(passed != Status::Success && pass != "exit" && passed != Status::ServerIsFull);
    m_colorChanger.setConsoleTextColor(Color::Default);
    if(pass == "exit"){
        passed = Status::Exit;
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return passed;
}

bool Client::sendClientDataToServer()
{
    sf::Packet packet;
    packet << m_client.m_name << m_client.m_type;
    if(m_client.m_socket.send(packet) != sf::Socket::Done){
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
        ("no-input", "Dont ask for input on start")
        ("username", "Set the username", cxxopts::value<std::string>())
        ("port", "Set the port", cxxopts::value<sf::Uint16>())
        ("ip", "Set server public ip", cxxopts::value<std::string>())
        ("h,help", "View this message")
    ;
    try
    {
        auto result = options.parse(argc, argv);
        if(result.count("help")){
            std::cout << options.help();
            return false;
        }
        if(result.count("no-input"))
            m_inputData = false;
        if(result.count("username"))
            m_client.m_name = result["username"].as<std::string>();
        if(result.count("ip"))
            m_serverIp = result["ip"].as<std::string>();
        if(result.count("port"))
            m_serverPort = result["port"].as<sf::Uint16>();
    }
    catch(std::exception& ex){
       printError(ex.what());
       return false;
    }

    return true;
}

void Client::printError(const std::string &l_string)
{
    Color tmp = m_colorChanger.m_color;
    m_colorChanger.setConsoleTextColor(Color::Red);
    std::cerr << l_string << std::endl;
    m_colorChanger.setConsoleTextColor(tmp);
}

void Client::printText(const std::string &l_string, const Color& l_color)
{
    Color tmp = m_colorChanger.m_color;
    m_colorChanger.setConsoleTextColor(l_color);
    std::cerr << l_string << std::endl;
    m_colorChanger.setConsoleTextColor(tmp);
}

void Client::run()
{
    m_running = true;

    std::thread receive(&Client::receiveThread, this);

    m_colorChanger.setConsoleTextColor(Color::White);
    while(m_running){
        std::string text;
        std::getline(std::cin, text);
        sendToServer(text);
    }
    m_colorChanger.setConsoleTextColor(Color::Default);

    receive.join();
}

void Client::receiveThread()
{
    while(m_running){
        sf::Packet packet;
        auto status = m_client.m_socket.receive(packet);
        if(status == sf::Socket::Done){
            unpack(packet);
        } else if(status == sf::Socket::Disconnected){
            printError("Server is not respodning");
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
}

void Client::unpack(sf::Packet &packet)
{
    Type type;
    packet >> type;
    switch(type){
        case Type::Message:{
            std::string username, message;
            ClientType type;
            packet >> type >> username >> message;
            Color color;
            std::string s;
            if(type == ClientType::Normie){
                color = Color::Default;
            } else if(type == ClientType::Administrator){
                color = Color::Green;
                s = "[ADMIN]";
            }
            s += ": ";
            printText(username + s + message, color);
            break;
        }
        case Type::ServerMessage:{
            std::string message;
            packet >> message;
            printText("[SERVER]: " + message, Color::Yellow);
            break;
        }
        case Type::Kick:{
            printError("You have been kicked from the server");
            m_running = false;
            break;
        }
        case Type::Connection:{
            std::string name;
            Type type2;
            Color color;
            packet >> name >> type2 >> color;
            std::string message;
            if(type2 == Type::Connection) message = " joined";
            else if(type2 == Type::Kick) message = " have been kicked";
            else if(type2 == Type::Disconnection) message = " disconnected";
            else message = " [unknown reason]";
            printText(name + message, color);
            break;
        }
        case Type::Promotion:{
            packet >> m_client.m_type;
            if(m_client.m_type == ClientType::Administrator){
                printText("You have been promoted to administrator", Color::Green);
            } else{
                printText("You have been degraded from administrator", Color::Red);
            }
            break;
        }
        case Type::Update:{
            ClientType type;
            std::string name;
            packet >> type >> name;
            if(type == ClientType::Administrator){
                printText(name + " have been promoted to administrator", Color::Green);
            } else{
                printText(name + " have been degraded from administrator", Color::Red);
            }
        }
    }
}

void Client::sendToServer(sf::Packet &l_packet)
{
    m_client.m_socket.send(l_packet);
}

void Client::sendToServer(const std::string &l_text)
{
    sf::Packet packet;
    packet << Type::Message << l_text;
    sendToServer(packet);
}
