#include "server.h"

int main(int argc, char**argv)
{
    Server server;
    if(!server.processArguments(argc, argv)){
       return 0;
    }
    server.printServerInfo();
    server.run();
}
