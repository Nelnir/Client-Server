#include <iostream>
#include "consoleclient.h"

int main(int argc, char** argv)
{
    ConsoleClient client;
    if(!client.processArguments(argc, argv)){
        return 0;
    }
    client.onInitialization();
    return client.run();
}
