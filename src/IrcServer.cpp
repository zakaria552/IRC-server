#include "IrcServer.hpp"
#include "Logger.hpp"
#include <arpa/inet.h>
#include <cstdlib>
#include <netdb.h>
#include <netinet/in.h>
#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <cstring>
#include <unistd.h>

IrcServer::IrcServer(const char *port, const char *password) : port(std::atoi(port)), password(password)
{
    struct addrinfo req{}, *res, *p;
    req.ai_family = AF_UNSPEC;
    req.ai_flags = AI_PASSIVE;
    if (getaddrinfo(nullptr, port, &req, &res) != 0)
    {
        Logger::error("Failed to retrieve host address information");
        std::runtime_error("Failed to retrieve host address information");
    }
    for (p = res; p != nullptr; p = p->ai_next)
    {
        if ((socketFd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) != -1)
        {
            Logger::info("AF: "+ std::to_string(p->ai_family) +
                        ", sockType: " + std::to_string(p->ai_socktype) +
                        ", AF_PROTOCAL: " + std::to_string(p->ai_protocol));
            break;
        }
    }
    if (bind(socketFd, p->ai_addr, p->ai_addrlen) != 0)
    {
        freeaddrinfo(res);
        Logger::error("Failed binding socket to address");
        std::runtime_error("Failed bindng socket to address");
    }
    freeaddrinfo(res);
}

void IrcServer::start()
{
    if (listen(socketFd, DEFAULT_BACKLOG) != 0)
    {
        Logger::error("Failed listening for connections on the socket");
        return;
    }
    struct sockaddr_storage clientAddr;
    socklen_t addrLen;
    int clientFd;
    char ip[INET_ADDRSTRLEN];
    char buff[1024];
    int dataLen;
    while (!closeConnection)
    {
       if ((clientFd = accept(socketFd, (struct sockaddr *)&clientAddr, &addrLen)) == -1)
       {
           Logger::warning("Accept failed...");
           continue;
       }
       if (clientAddr.ss_family != AF_INET && clientAddr.ss_family != AF_INET6)
       {
           Logger::warning("Unknown address family: " + std::to_string(clientAddr.ss_family));
           continue;
       }
       inet_ntop(clientAddr.ss_family, (struct sockaddr *)&clientAddr, ip, sizeof(ip));
       Logger::info("Accepting connection ipv4: " + std::string(ip));
       dataLen = recv(clientFd, buff, sizeof(buff), 0);
       if (dataLen == 0)
           Logger::warning("Client closed connection"), close(clientFd);
       else if (dataLen == -1)
           Logger::warning("Something went wrong reading from client");
       else
           Logger::info("Data received: " + std::to_string(dataLen) + "byte - " + std::string(buff));
       std::memset(buff, 0, 1024);
    }
}
