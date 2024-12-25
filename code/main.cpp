#include "webserver/server.h"

int main()
{
    WebServer server(8080, 8);
    std::cout << "server is running" << std::endl;
    server.start();
    return 0;
}