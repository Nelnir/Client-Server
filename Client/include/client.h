#ifndef CLIENT_H
#define CLIENT_H

#include <SFML/Network.hpp>
#include "../../Shared/shared.h"
#include <string>

enum class Status { ServerIsFull, ErrorWhenRetrievingData, ErrorWhenSendingData, Success, Wrong, Exit, UnableToConnect, Blocked};

class Client
{
    ColorChanger m_colorChanger;
public:
    Client();
    ~Client();


    void inputData();
    bool processArguments(int& argc, char**& argv);

    Status connect();
    void successfullyConnected();
    void run();
    void printError(const std::string &l_string);
    void printText(const std::string& l_string, const Color& l_color);
private:
    void receiveThread();
    Shared m_shared;
    ClientData m_client;
    sf::IpAddress m_serverIp;
    sf::Uint16 m_serverPort;
    bool m_inputData;
    std::string m_version;
    bool m_running;

    void unpack(sf::Packet& l_packet);

    void sendToServer(const std::string& l_text);
    void sendToServer(sf::Packet& l_packet);
    bool sendClientDataToServer();
    Status inputServerPassword();
    Status checkServerPassword(const std::string& l_pass);
};

#endif // CLIENT_H
