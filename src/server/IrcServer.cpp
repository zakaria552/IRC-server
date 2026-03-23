#include "IrcServer.hpp"
#include "Logger.hpp"
#include <stdexcept>
#include <cstring>

IrcServer::IrcServer(const char *port, const char *password) :  password(password)
{
    struct addrinfo req{}, *res, *p;
    req.ai_family = AF_UNSPEC;
    req.ai_flags = AI_PASSIVE;
    if (getaddrinfo(nullptr, port, &req, &res) != 0)
        throw std::runtime_error("Failed to retrieve host address information");
    for (p = res; p != nullptr; p = p->ai_next)
    {
        if (p->ai_protocol == AF_ROUTE)
            continue;
        if ((socketFd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) != -1)
        {
            Logger::info("AF: "+ std::to_string(p->ai_family) +
                        ", sockType: " + std::to_string(p->ai_socktype) +
                        ", AF_PROTOCAL: " + std::to_string(p->ai_protocol));
            break;
        }
    }
    if (socketFd < 0)
    {
        freeaddrinfo(res);
        throw std::runtime_error("Failed to create socket");
    }
    if (bind(socketFd, p->ai_addr, p->ai_addrlen) != 0)
    {
        freeaddrinfo(res);
        throw std::runtime_error("Failed binding socket to address");
    }
    freeaddrinfo(res);
    pollfd newPoll = {};
    newPoll.fd = socketFd;
    newPoll.events = POLLIN;
    ioEvents.add(newPoll);
}

void IrcServer::start()
{
    if (listen(socketFd, DEFAULT_BACKLOG) != 0)
        throw std::runtime_error("Failed to listen for connections on the socket" + std::string(strerror(errno)));
    while (!closeConnection)
    {
        ioEvents.pollEvents();
        int i = 0;
        for (auto client: ioEvents)
        {
            Logger::info(std::to_string(client.fd));
            if (!(client.revents & POLLIN) && ++i)
                continue;
            if (client.fd == socketFd && ++i)
            {
                newClient();
                continue;
            }
            char buff[1024];
            int n = read(client.fd, buff, 1024);
            if (n > 0)
                processRequest(client.fd, buff, n);
            else if (n == 0)
                clientDisconnected(i);
            else
                Logger::warning("Read failed");
            i++;
        }
    }
}

void IrcServer::newClient()
{
    Logger::info("new connection");
    struct sockaddr_storage clientAddr;
    pollfd newPoll = {};
    socklen_t addrLen;
    int clientFd;
    if ((clientFd = accept(socketFd, (struct sockaddr *)&clientAddr, &addrLen)) < 0)
    {
        Logger::warning("Failed to accept client");
        return;
    }
    newPoll.fd = clientFd;
    newPoll.events = POLLIN;
    ioEvents.add(newPoll);
}

void IrcServer::processRequest(const int clientFd, const char *body, const size_t length)
{
    Logger::info("Processing client request");
    send(clientFd, body, length, MSG_DONTWAIT | MSG_NOSIGNAL);
}

void IrcServer::clientDisconnected(const int index)
{
    Logger::info("Client disconnected");
    ioEvents.remove(index);
}
