#pragma once
#include <netdb.h>
#include <string>
#include <sys/socket.h>

class IrcServer
{
    int port;
    std::string password; // tmp
public:
    IrcServer() = delete;
    IrcServer(const int &port, const char *password);
};
