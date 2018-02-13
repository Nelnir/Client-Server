#ifndef CONSOLECLIENT_H
#define CONSOLECLIENT_H

#include "client.h"

class ConsoleClient : public Client
{
    ColorChanger m_colorChanger;
public:
    ConsoleClient();
    ~ConsoleClient();

    int run();

    void printError(const std::string &l_string);
    void printText(const std::string& l_string, const Color& l_color);

    void onInitialization();
private:
    void inputThread();
protected:
    std::string onServerPasswordNeeded();

    void onSuccessfullyConnected();
    void onErrorWithSendingData();
    void onErrorWithReceivingData();
    void onDisconnected();
    void onServerWrongPassword();
    void onArgumentsError(const char *);
    void onUnableToConnect();
    void onServerIsFull();
    void onBlockedFromServer();
    void onError(const std::string &l_text);


    void onMessageReceived(const std::string&, const std::string&, const ClientType& l_type);
    void onServerMessageReceived(const std::string&);
    void onKick();
    void onPromotion(const std::string& l_text, const bool& l_promotion);
    void onConnectionNotificationReceived(const std::string&, const Type&);
    void onServerExit();
};

#endif // CONSOLECLIENT_H
