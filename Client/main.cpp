#include <iostream>
#include "client.h"

int main(int argc, char** argv)
{
    Client client;
    if(!client.processArguments(argc, argv)){
        return 0;
    }
    client.inputData();
    std::cout << "Connecting..." << std::endl;
    switch(client.connect())
    {
    case Status::Success: client.successfullyConnected(); client.run(); return 0; break;
    case Status::ServerIsFull: client.printError("Server is full"); break;
    case Status::ErrorWhenRetrievingData: client.printError("Error when retrieving data from server"); break;
    case Status::ErrorWhenSendingData: client.printError("Error when sending data to server"); break;
    case Status::UnableToConnect: client.printError("Unable to connect"); break;
    case Status::Blocked: client.printError("You are blocked on this server"); break;
    case Status::Exit: return 0;
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();
    return 0;
}
