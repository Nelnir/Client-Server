#ifndef CONSOLESERVER_H
#define CONSOLESERVER_H

#include "server.h"

using Commands = std::map<std::string, std::function<void()>>;
using CommandsDescriptions = std::map<std::string, std::string>;

class ConsoleServer : public Server
{
public:
    ConsoleServer();
    ~ConsoleServer();

    int run();

    void printError(const std::string& l_string);
    void printText(const std::string& l_string, const Color& l_color);
    void printServerInfo();

private:
    Commands m_commands;
    CommandsDescriptions m_commandsDescriptions;
    ColorChanger m_colorChanger;

    void inputThread();

    /// COMMANDS
    void changeMaxClients();
    void changePassword();
    void viewAllCommands();
    void viewAllClients();
    void sendMessage();
    void kick();
    void block();
    void unblock();
    void promote();
    void helpPromote();

    void printClientMessage(std::unique_ptr<ClientServerData> &l_data, const std::string &l_message);

    /// input function
    std::string getline();

protected:
    void onClientBlocked(std::unique_ptr<ClientServerData>& l_client);
    void onClientRejected(std::unique_ptr<ClientServerData>& l_client);
    void onClientConnected(std::unique_ptr<ClientServerData>& l_client);
    void onClientDisconnected(std::unique_ptr<ClientServerData>& l_client);
    void onClientPromoted(std::unique_ptr<ClientServerData>& l_client, const bool& l_promoted);
    void onClientMessageReceived(std::unique_ptr<ClientServerData> &l_client, const std::string &l_text);
    void onErrorWithReceivingData(std::unique_ptr<ClientServerData>& l_client);
    void onErrorWithSendingData(std::unique_ptr<ClientServerData>& l_client);
    void onArgumentsError(const char * l_what);
    void error(const std::string& l_text);
};

#endif // CONSOLESERVER_H
