#include "IrcServer.hpp"
#include "commands/IrcCommand.hpp"
#include "commands/IrcCommands.hpp"
#include "parser/RawCommandParser.hpp"
#include "parser/CommandParser.hpp"
#include "server/Client.hpp"
#include "utils/Logger.hpp"
#include <cerrno>
#include <optional>
#include <stdexcept>
#include <cstring>
#include <string>

IrcServer::IrcServer(const char *port, const char *password) :  password(password)
{
    struct addrinfo req{}, *res, *p;
    req.ai_family = AF_INET;
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
    int optval = 1;
    if (setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval) != 0)
        throw std::runtime_error("Failed to set socket option: " + std::string(strerror(errno)));
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
            if (!(client.revents & POLLIN) && ++i)
                continue;
            if (client.fd == socketFd && ++i)
            {
                newClient();
                continue;
            }
            char buff[1024];
            int n = recv(client.fd, buff, 1024, MSG_DONTWAIT);
            if (n > 0)
                processRequest(client.fd, buff, n);
            else if (n == 0)
                clientDisconnected(i);
            else
                Logger::warning("Recv failed");
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
    clients[clientFd] = Client(clientFd);
    newPoll.fd = clientFd;
    newPoll.events = POLLIN;
    ioEvents.add(newPoll);
}

void IrcServer::processRequest(const int clientFd, const char *body, const size_t length)
{
    Logger::info("Processing client request");
    RawIrcCommands msgs = parser.parse(clientFd, body, length);
    std::queue<IrcCommand> cmds = translateRawCommands(msgs);
    if (cmds.empty())
        return;
    while(!cmds.empty())
    {
        switch (cmds.front().type) {
            case IrcCommand::CAP:
                Logger::warning("Ignoring capability handshake");
                break;
            case IrcCommand::NICK:
            {
                std::string nick = cmds.front().payload.nick.nickname;
                clients[clientFd].setNick(nick);
                if (!authenticate(clients[clientFd]))
                {
                    Logger::info("Failed to authenticate client, booting them off from server");
                    clientDisconnected(clientFd);
                    return;
                }
                Logger::info("Nic: " + clients[clientFd].getNick());
                std::string body = "001 " + clients[clientFd].getNick() + " :Welcome to the Internet Relay Network " + clients[clientFd].getNick() + "\r\n";
                send(clientFd, body.c_str(), body.length(), MSG_DONTWAIT | MSG_NOSIGNAL);
                break;
            }
            case IrcCommand::PASS:
            {
                clients[clientFd].setPass(cmds.front().payload.pass.password);
                break;
            }
            case IrcCommand::JOIN:
            {
                std::string c = cmds.front().payload.join.channels;
                channels.add(c.substr(1), clientFd);
                break;
            }
            case IrcCommand::PRIVMSG:
            {
                std::string msg = cmds.front().payload.privmsg.say_text;
                std::string target = cmds.front().payload.privmsg.targets;
                channels.sendMessage(clients[clientFd], target, msg);
                break;
            }
            default:
                break;
        }
        cmds.pop();
    }
}

void IrcServer::clientDisconnected(const unsigned int &clientFd)
{
    Logger::info("Client disconnected");
    clients.erase(clientFd);
    ioEvents.remove(clientFd);
    close(clientFd);
}

std::queue<IrcCommand> IrcServer::translateRawCommands(RawIrcCommands& raws)
{
    CommandParser p = CommandParser();
    std::queue<IrcCommand> cmds;

    while (not raws.empty()) {
        auto const& raw = raws.front();

        std::optional<IrcCommand> cmd = p.Parse(raw);
        if (not cmd.has_value())
        {
            Logger::info("Dropped raw cmd: " + raw.cmd);
        }
        else
        {
            cmds.push(std::move(cmd.value()));
        }

        raws.pop();
    }

    return cmds;
}

bool IrcServer::authenticate(const Client &client)
{
    return client.getPass() == password;
}
