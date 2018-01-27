#include "consoleserver.h"

int main(int argc, char**argv)
{
    ConsoleServer server;
    if(!server.processArguments(argc, argv)){
       return 0;
    }
    return server.run();
}
