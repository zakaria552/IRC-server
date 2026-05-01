#include "IrcServer.hpp"
#include "commands/IrcCommand.hpp"
#include "commands/IrcCommands.hpp"
#include "parser/RawCommandParser.hpp"
#include "parser/CommandParser.hpp"
#include "server/Channel.hpp"
#include "server/ChannelsManager.hpp"
#include "server/Client.hpp"
#include "server/NumericReplies.hpp"
#include "server/QueueMessages.hpp"
#include "utils/Logger.hpp"
#include <cerrno>
#include <optional>
#include <stdexcept>
#include <cstring>
#include <string>
#include "server/globals.hpp"

IrcServer::IrcServer(const std::string &name, const char *port, const char *password)
    : password(password), queueBroadcastMessages(), channels(queueBroadcastMessages)
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
    serverName = name;
}

void IrcServer::start()
{
    if (listen(socketFd, DEFAULT_BACKLOG) != 0)
        throw std::runtime_error("Failed to listen for connections on the socket" + std::string(strerror(errno)));
    while (!closeConnection)
    {
        ioEvents.pollEvents();
        for (auto client: ioEvents)
        {
            if (not (client.revents & POLLIN))
                continue;
            if (client.fd == socketFd)
            {
                newClient();
                continue;
            }
            char buff[1024];
            int n = recv(client.fd, buff, 1024, MSG_DONTWAIT);
            if (n > 0)
                processRequest(client.fd, buff, n);
            else if (n == 0)
                clientDisconnected(client.fd);
            else
                Logger::warning("Recv failed");
        }
        flushMsgQueues();
    }
}

void IrcServer::newClient()
{
    Logger::info("new connection");
    sockaddr_storage clientAddr;
    pollfd newPoll = {};
    socklen_t addrLen = sizeof(sockaddr_storage);
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

void IrcServer::processRequest(int clientFd, const char *body, const size_t length)
{
    Logger::info("Processing client request");
    RawIrcCommands msgs = parser.parse(clientFd, body, length);
    std::queue<IrcCommand> cmds = translateRawCommands(msgs, clientFd);
    if (cmds.empty())
        return;
    while(!cmds.empty())
    {
        switch (cmds.front().type) {
            case IrcCommand::UNDEFINED:
                Logger::warning("Undefined cmd");
                break;
            case IrcCommand::CAP:
                Logger::warning("Ignoring capability handshake");
                break;
            case IrcCommand::NICK:
                HandleNickCmd(cmds.front().payload.nick);
                break;
            case IrcCommand::PASS:
                HandlePassCmd(cmds.front().payload.pass);
                break;
            case IrcCommand::JOIN:
                HandleJoinCmd(cmds.front().payload.join);
                break;
            case IrcCommand::PRIVMSG:
                HandlePrivMsgCmd(cmds.front().payload.privmsg);
                break;
            case IrcCommand::PING:
                HandlePingCmd(cmds.front().payload.ping);
                break;
            case IrcCommand::USER:
                HandleUserCmd(cmds.front().payload.user);
                break;
            case IrcCommand::INVITE:
                HandleInviteCmd(cmds.front().payload.invite);
                break;
            case IrcCommand::MODE:
                HandleModeCmd(cmds.front().payload.mode);
                break;
        }
        cmds.pop();
    }
}
// [TODO] remove stale messages
// [TODO] remove client from channels
void IrcServer::clientDisconnected(int clientFd)
{
    Logger::info("Client disconnected");

    // .erase() was sometimes returning 0 instead of expected 1 element removed.
    if (clients.erase(clientFd) == 0) [[unlikely]]
    {
        Logger::info("UNEXPECTED FAIL TO ERASE CLIENT!");
        std::exit(1);
    }

    ioEvents.remove(clientFd);
    close(clientFd);
}

std::queue<IrcCommand> IrcServer::translateRawCommands(RawIrcCommands& raws, int clientFd)
{
    CommandParser p = CommandParser();
    std::queue<IrcCommand> cmds;

    while (not raws.empty()) {
        auto const& raw = raws.front();

        std::optional<IrcCommand> cmd = p.Parse(raw, clientFd);
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

void IrcServer::flushMsgQueues()
{
    while (!queueMessages.empty())
    {
        const Message &msg = queueMessages.front();
        if (send(msg.clientFd, msg.msg.c_str(), msg.msg.length(), MSG_DONTWAIT | MSG_NOSIGNAL) == -1)
            return;
        queueMessages.pop();
    }
    while (!queueBroadcastMessages.empty())
    {
        BroadcastMessage &msg = queueBroadcastMessages.front();
        for (size_t i = msg.totalSent; i < msg.clientFds.size(); i++)
        {
            if (send(msg.clientFds[i], msg.msg.c_str(), msg.msg.length(), MSG_DONTWAIT | MSG_NOSIGNAL) == -1)
                return;
            msg.totalSent++;
        }
        queueBroadcastMessages.pop();
    }
}

// Handlers
void IrcServer::HandlePingCmd(const IrcCommand::PingCmd &cmd)
{
    std::string src = ":";
    std::string body = src + serverName + " " + cmd.token + "\r\n";
    queueMessages.push({cmd.client, body});
}

void IrcServer::HandlePrivMsgCmd(const IrcCommand::PrivMsgCmd &cmd)
{
    if (cmd.targets[0] == '#')
    {
        Channel *channel = channels.getChannel(cmd.targets.substr(1));
        if (channel)
            queueBroadcastMessages.push(channel->constructMessage(clients[cmd.client], cmd.say_text));
        return;
    }
    for(auto [fd, client]: clients)
    {
        std::string_view nick = client.getNick();
        if (fd != cmd.client && nick == cmd.targets)
        {
            std::string src = ":" + clients[cmd.client].getNick();
            std::string body = src + " PRIVMSG " + cmd.targets + " :" + cmd.say_text + "\r\n";
            queueMessages.push({fd, body});
            return;
        }
    }
    Logger::info("Not found user to send the message");
}

void IrcServer::HandleInviteCmd(const IrcCommand::InviteCmd &cmd)
{
    Channel *channel = channels.getChannel(cmd.channel);
    if (!channel)
        return queueMessages.push(NumericReplies::channelNotFound(cmd.channel, clients[cmd.client]));
    if (!channel->isMember(cmd.client))
        return queueMessages.push(NumericReplies::notChannelMember(cmd.channel, clients[cmd.client]));
    for(auto [fd, client]:clients)
    {
        const std::string nick = client.getNick();
        if (fd != cmd.client && nick == cmd.nick)
        {
            if (channel->isMember(fd))
                return queueMessages.push(NumericReplies::isChannelMember(cmd.channel, clients[cmd.client], client));
            std::string src = ":" + clients[cmd.client].getNick();
            std::string body = src + " INVITE " + nick + " :#" + cmd.channel + "\r\n";
            channel->invite(cmd.nick);
            queueMessages.push({fd, body});
            return;
        }
    }
}

void IrcServer::HandleModeCmd(const IrcCommand::ModeCmd &cmd)
{
    if (cmd.target[0] != '#')
        return;
    const std::string channelName = cmd.target.substr(1);
    if (!channels.channelExist(channelName))
        return queueMessages.push(NumericReplies::channelNotFound(channelName, clients[cmd.client]));
    if (!channels.isMemberOfChannel(channelName, cmd.client))
        return queueMessages.push(NumericReplies::notChannelMember(channelName, clients[cmd.client]));
    Channel *channel = channels.getChannel(channelName);
    if (cmd.mode == NONE)
        return queueMessages.push(NumericReplies::listModes(channelName, channel->listModes(), clients[cmd.client]));
    if (!channel->isOperator(cmd.client))
        return queueMessages.push(NumericReplies::isNotOperator(channelName, clients[cmd.client]));
    switch (cmd.mode) {
        case INVITE_ONLY:
        {
            channels.updateChannelMode(channelName, INVITE_ONLY, cmd.intent);
            channels.broadcastModeChange(clients[cmd.client], channelName, cmd.raw);
            break;
        }
        case REQUIRE_PASS:
        {
            if (cmd.intent == '+' && cmd.key.empty())
                return; // [TODO] handle invalid mode params
            channels.updateChannelMode(channelName, REQUIRE_PASS, cmd.intent);
            channel->setKey(cmd.key);
            channels.broadcastModeChange(clients[cmd.client], channelName, cmd.raw);
            break;
        }
        case USER_LIMIT:
        {
            if (cmd.maxUser < 0)
                return queueMessages.push(NumericReplies::invalidModeParams(channelName, clients[cmd.client], "l " + std::to_string(cmd.maxUser), "Can't be negative"));
            channels.updateChannelMode(channelName, USER_LIMIT, cmd.intent);
            if (cmd.intent == '+')
                channel->setMaxUserLimit(cmd.maxUser);
            channels.broadcastModeChange(clients[cmd.client], channelName, cmd.raw);
            break;
        }
        case RESTRICT_TOPIC:
        {
            channels.updateChannelMode(channelName, RESTRICT_TOPIC, cmd.intent);
            channels.broadcastModeChange(clients[cmd.client], channelName, cmd.raw);
            break;
        }
        case OP_PRIVILEGE:
        {
            Client *target = getClientByNick(cmd.nick);
            if (!target)
               return queueMessages.push(NumericReplies::noSuchUser(clients[cmd.client], cmd.nick));
            if (!channels.isMemberOfChannel(channelName, target->getSocket()))
                return queueMessages.push(NumericReplies::userNotInChannel(channelName, clients[cmd.client], *target));
            channel->updateOperators(target->getSocket(), cmd.intent == '+');
            channels.broadcastModeChange(clients[cmd.client], channelName, cmd.raw);
            break;
        }
        default:
            break;
    }
}

void IrcServer::HandleJoinCmd(const IrcCommand::JoinCmd &cmd)
{
    Client &client = clients[cmd.client];
    for(size_t i = 0; i < cmd.channels.size(); i++)
    {
        std::string channelName = cmd.channels[i].substr(1);
        Channel *channel = channels.getChannel(channelName);
        if (channel && channel->isMember(cmd.client))
            continue;
        if (channel && channel->modeIsSet(INVITE_ONLY) && !channel->isInvited(client.getNick()))
        {
            queueMessages.push(NumericReplies::isInviteOnly(channelName, clients[cmd.client]));
            continue;
        }
        if (channel && channel->modeIsSet(REQUIRE_PASS) && (cmd.keys.size() <= i || !channel->isValidKey(cmd.keys[i])))
        {
            queueMessages.push(NumericReplies::invalidChannelKey(channelName, clients[cmd.client]));
            continue;
        }
        if (channel && channel->modeIsSet(USER_LIMIT) && channel->isFull())
        {
            queueMessages.push(NumericReplies::channelIsFull(channelName, clients[cmd.client]));
            continue;
        }
        if (!channel)
            channel = channels.newChannel(channelName);
        else if (channel->isBlackListed(cmd.client))
            continue; // [TODO] handle
        channel->addClient(cmd.client);
        channels.broadcastJoinedUser(client, channelName);
        sendListOfUsers(client, channel);
    }
}

void IrcServer::HandleNickCmd(const IrcCommand::NickCmd &cmd)
{
    std::string nick = cmd.nickname;
    clients[cmd.client].setNick(nick);
    if (!authenticate(clients[cmd.client]))
    {
        Logger::info("Failed to authenticate client, booting them off from server");
        std::string body = NumericReplies::passMisMatch();
        queueMessages.push({cmd.client, body});
        clientDisconnected(cmd.client);
        return;
    }
    std::string body = NumericReplies::welcome() + clients[cmd.client].getNick() + " :Welcome to the Internet Relay Network " + clients[cmd.client].getNick() + "\r\n";
    queueMessages.push({cmd.client, body});
}

void IrcServer::HandlePassCmd(const IrcCommand::PassCmd &cmd)
{
    clients[cmd.client].setPass(cmd.password);
}

void IrcServer::HandleCapCmd(const IrcCommand::CapCmd &cmd)
{
   (void) cmd;
   Logger::warning("Ignoring capability handshake");
}

void IrcServer::HandleUserCmd(const IrcCommand::UserCmd &cmd)
{
    clients[cmd.client].setFullname(cmd.fullName);
    clients[cmd.client].setUsername(cmd.user);
}

void IrcServer::sendListOfUsers(const Client &client, Channel *channel)
{
    Message message; //"<client> <symbol> <channel> :[prefix]<nick>{ [prefix]<nick>}"
    const std::string body = ":" + serverName + " 353 " + client.getNick() + " =" + " #" + channel->getName() + " :";
    const std::vector<int> users = channel->getClients();
    message.clientFd = client.getSocket();
    for(size_t i = 0; i < users.size(); i++)
    {
        Client &member = clients[users[i]];
        std::string opPrefix = channel->isOperator(member.getSocket()) ? "@" : "";
        message.msg = body + opPrefix + member.getNick() + "\r\n";
        queueMessages.push(message);
    }
    //:server 366 alice #chat :End of /NAMES list
    message.msg = ":" + serverName + " 366 " + client.getNick() + " #" + channel->getName() + " :End of /NAMES list\r\n";
    queueMessages.push(message);
}

Client *IrcServer::getClientByNick(const std::string &nick)
{
    for(auto &[fd, client] : clients)
    {
        if (client.getNick() == nick)
            return &client;
    }
    return nullptr;
}
