#include "consoleclient.h"
#include <iostream>
#include <thread>

ConsoleClient::ConsoleClient()
{

}

ConsoleClient::~ConsoleClient()
{

}

int ConsoleClient::run()
{
    m_running = true;
    std::thread(&ConsoleClient::inputThread, this).detach();
    return Client::run();
}

void ConsoleClient::inputThread()
{
    m_colorChanger.setConsoleTextColor(Color::White);
    while(m_running){
        std::string text;
        std::getline(std::cin, text);
        sendToServer(text);
    }
    m_colorChanger.setConsoleTextColor(Color::Default);
}

void ConsoleClient::onInitialization()
{
    m_colorChanger.setConsoleTextColor(Color::White);
    std::cout << "Username: ";    std::cin >> m_client.m_name;
    std::cout << "Server ip: ";   std::cin >> m_serverIp;
    std::cout << "Server port: "; std::cin >> m_serverPort;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    m_colorChanger.setConsoleTextColor(Color::Default);
}

void ConsoleClient::onSuccessfullyConnected()
{
    printText("Successfully connected", Color::Green);
}

void ConsoleClient::onErrorWithSendingData()
{
    printError("An error has occured with sending data");
}

void ConsoleClient::onErrorWithReceivingData()
{
    printError("An error has occured with receiving data");
}

void ConsoleClient::onServerClosedConnection()
{
    printError("Server closed the connection");
    std::this_thread::sleep_for(std::chrono::seconds(3));
}

void ConsoleClient::onArgumentsError(const char *l_what)
{
    printError(l_what);
}

void ConsoleClient::onUnableToConnect()
{
    printError("Unable to connect");
    std::this_thread::sleep_for(std::chrono::seconds(3));
}

void ConsoleClient::onServerIsFull()
{
    printError("Server is full");
    std::this_thread::sleep_for(std::chrono::seconds(3));
}

void ConsoleClient::onBlockedFromServer()
{
    printError("You are blocked from this server");
    std::this_thread::sleep_for(std::chrono::seconds(3));
}

void ConsoleClient::onError(const std::string &l_text)
{
    printError(l_text);
}

std::string ConsoleClient::onServerPasswordNeeded()
{
    std::string password;
    std::cout << "Password: ";
    m_colorChanger.setConsoleTextColor(Color::White);
    std::cin >> password;
    m_colorChanger.setConsoleTextColor(Color::Default);
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return password;
}

void ConsoleClient::onServerWrongPassword()
{
    printError("Wrong password");
}

void ConsoleClient::printError(const std::string &l_string)
{
    Color tmp = m_colorChanger.m_color;
    m_colorChanger.setConsoleTextColor(Color::Red);
    std::cerr << l_string << std::endl;
    m_colorChanger.setConsoleTextColor(tmp);
}

void ConsoleClient::printText(const std::string &l_string, const Color &l_color)
{
    Color tmp = m_colorChanger.m_color;
    m_colorChanger.setConsoleTextColor(l_color);
    std::cout << l_string << std::endl;
    m_colorChanger.setConsoleTextColor(tmp);
}

void ConsoleClient::onMessageReceived(const std::string & l_string, const ClientType& l_type)
{
    if(l_type == ClientType::Normie){
        printText(l_string, Color::Default);
    } else {
        printText(l_string, Color::Green);
    }
}

void ConsoleClient::onServerMessageReceived(const std::string & l_string)
{
    printText(l_string, Color::Yellow);
}

void ConsoleClient::onKick()
{
    printText("You have been kicked", Color::Red);
    std::this_thread::sleep_for(std::chrono::seconds(3));
}

void ConsoleClient::onConnectionNotificationReceived(const std::string & l_string, const Type & l_type)
{
    Color color;
    if(l_type == Type::Connection){
        color = Color::Green;
    } else {
        color = Color::Red;
    }
    printText(l_string, color);
}

void ConsoleClient::onPromotion(const std::string &l_text, const bool &l_promotion)
{
    printText(l_text, l_promotion ? Color::Green : Color::Red);
}

void ConsoleClient::onServerExit()
{
    printText("Server closed the connection", Color::Red);
    std::this_thread::sleep_for(std::chrono::seconds(3));
}
