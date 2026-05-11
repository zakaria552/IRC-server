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
#include <charconv>
#include <optional>
#include <stdexcept>
#include <cstring>
#include <string>
#include <sys/socket.h>
#include <system_error>
#include "server/globals.hpp"

IrcServer::IrcServer(const std::string &name, const char *port, const char *password)
    : password(password), queueBroadcastMessages(), channels(queueBroadcastMessages)
{
    struct addrinfo req{}, *res, *p;
    req.ai_family = AF_INET;
    req.ai_flags = AI_PASSIVE;
    Logger::info("Initializing server...");
    if (getaddrinfo(nullptr, port, &req, &res) != 0)
        throw std::runtime_error("Failed to retrieve host address information");
    for (p = res; p != nullptr; p = p->ai_next)
    {
        if (p->ai_protocol == AF_ROUTE)
            continue;
        if ((socketFd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) != -1)
        {
            Logger::debug("AF: "+ std::to_string(p->ai_family) +
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
    Logger::info("Starting sever");
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
    sockaddr clientAddr;
    pollfd newPoll = {};
    socklen_t addrLen = sizeof(clientAddr);
    char ipstr[INET_ADDRSTRLEN];
    int clientFd;
    if ((clientFd = accept(socketFd, &clientAddr, &addrLen)) < 0)
    {
        Logger::warning("Failed to accept client: " + std::string(std::strerror(errno)));
        return;
    }
    if (inet_ntop(clientAddr.sa_family, &clientAddr, ipstr, sizeof(ipstr)) == NULL)
    {
        Logger::warning("Failed retrieve client's IP address: " + std::string(std::strerror(errno)));
        close(clientFd);
        return;
    }
    clients[clientFd] = Client(clientFd, ipstr);
    newPoll.fd = clientFd;
    newPoll.events = POLLIN;
    ioEvents.add(newPoll);
    Logger::info("New client from: " + clients[clientFd].getIpAddress());
}

void IrcServer::processRequest(int clientFd, const char *body, const size_t length)
{
    Logger::debug("Processing client request");
    RawIrcCommands msgs = parser.parse(clientFd, body, length);
    std::queue<IrcCommand> cmds = translateRawCommands(msgs, clientFd);
    if (cmds.empty())
        return;
    while(!cmds.empty())
    {
        switch (cmds.front().type) {
            case IrcCommand::UNDEFINED:
                Logger::debug("Undefined cmd");
                break;
            case IrcCommand::CAP:
                HandleCapCmd(cmds.front().payload.cap);
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
            case IrcCommand::TOPIC:
                HandleTopicCmd(cmds.front().payload.topic);
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
        Logger::error("UNEXPECTED FAIL TO ERASE CLIENT!");
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
            Logger::debug("Dropped raw cmd: " + raw.cmd);
        }
        else
        {
            cmds.push(std::move(cmd.value()));
        }

        raws.pop();
    }

    return cmds;
}

void IrcServer::authenticate(Client &client)
{
    if (!password.empty() && (client.getPass() != password))
    {
        Logger::debug("Failed to authenticate client, booting them off from server");
        std::string body = NumericReplies::passMisMatch();
        queueMessages.push({client.getSocket(), body});
        clientDisconnected(client.getSocket());
        return;
    }
    std::string body = NumericReplies::welcome() + client.getNick() + " :Welcome to the Internet Relay Network " + client.getNick() + "\r\n";
    queueMessages.push({client.getSocket(), body});
    client.updateHandshakeState(Client::Handshake::AUTHENTICATED);
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
    Client &client = clients[cmd.client];
    if (!client.isAuthenticated())
        return;
    std::string src = ":";
    std::string body = src + serverName + " " + cmd.token + "\r\n";
    queueMessages.push({cmd.client, body});
}

void IrcServer::HandlePrivMsgCmd(const IrcCommand::PrivMsgCmd &cmd)
{
    Client &client = clients[cmd.client];
    if (!client.isAuthenticated())
        return;
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
    Logger::debug("Not found user to send the message");
}

void IrcServer::HandleInviteCmd(const IrcCommand::InviteCmd &cmd)
{
    Client &client = clients[cmd.client];
    if (!client.isAuthenticated())
        return;
    Channel *channel = channels.getChannel(cmd.channel);
    if (!channel)
    {
        queueMessages.push(NumericReplies::channelNotFound(cmd.channel, clients[cmd.client]));
        return;
    }
    if (!channel->isMember(cmd.client))
    {
        queueMessages.push(NumericReplies::notChannelMember(cmd.channel, clients[cmd.client]));
        return;
    }
    for(auto [fd, client]:clients)
    {
        const std::string nick = client.getNick();
        if (fd != cmd.client && nick == cmd.nick)
        {
            if (channel->isMember(fd))
            {
                queueMessages.push(NumericReplies::isChannelMember(cmd.channel, clients[cmd.client], client));
                return;
            }
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
    Client &client = clients[cmd.client];
    if (!client.isAuthenticated())
        return;
    if (cmd.target[0] != '#')
        return;
    const std::string channelName = cmd.target.substr(1);
    if (!channels.channelExist(channelName))
    {
        queueMessages.push(NumericReplies::channelNotFound(channelName, clients[cmd.client]));
        return;
    }
    if (!channels.isMemberOfChannel(channelName, cmd.client))
    {
        queueMessages.push(NumericReplies::notChannelMember(channelName, clients[cmd.client]));
        return;
    }
    Channel *channel = channels.getChannel(channelName);
    if (cmd.listOfModes.empty())
    {
        queueMessages.push(NumericReplies::listModes(channelName, channel->listModes(), clients[cmd.client]));
        return;
    }
    if (!channel->isOperator(cmd.client))
    {
        queueMessages.push(NumericReplies::isNotOperator(channelName, clients[cmd.client]));
        return;
    }
    std::string strModes = "MODE " + cmd.target + " ";
    std::string params;
    bool shouldBroadcastModeChange = false;
    for(size_t i = 0, j = 0; i < cmd.listOfModes.size(); i++)
    {
        char intent = cmd.listOfModes[i].intent;
        std::string strMode;
        for(size_t k = 0; k < cmd.listOfModes[i].modes.size(); k++)
        {
            Channel::Mode mode = cmd.listOfModes[i].modes[k];
            switch (mode) {
                case Channel::Mode::INVITE_ONLY:
                {
                    channels.updateChannelMode(channelName, mode, intent);
                    strMode += 'i';
                    break;
                }
                case Channel::Mode::REQUIRE_PASS:
                {
                    if (intent == '+' && cmd.params.size() < (j + 1))
                        continue; // [TODO] handle invalid mode params
                    if (intent == '+')
                    {
                        channel->setKey(cmd.params[j]);
                        j++;
                    }
                    channels.updateChannelMode(channelName, mode, intent);
                    params += " " + channel->getKey();
                    strMode += 'k';
                    break;
                }
                case Channel::Mode::USER_LIMIT:
                {
                    if (intent == '+' && cmd.params.size() < (j + 1))
                        continue; // [TODO] handle invalid mode params
                    if (intent == '+')
                    {
                        int maxUser;
                        const char *param = cmd.params[j].c_str();
                        std::from_chars_result res = std::from_chars(param, param + cmd.params[j].size(), maxUser);
                        j++;
                        if (res.ec == std::errc::invalid_argument || res.ec == std::errc::result_out_of_range || maxUser < 0)
                        {
                            queueMessages.push(NumericReplies::invalidModeParams(channelName, clients[cmd.client], "l " + cmd.params[j-1], "Invalid argument"));
                            continue;
                        }
                        channel->setMaxUserLimit(maxUser);
                        params += " " + std::to_string(maxUser);
                    }
                    channels.updateChannelMode(channelName, mode, intent);
                    strMode += 'l';
                    break;
                }
                case Channel::Mode::RESTRICT_TOPIC:
                {
                    channels.updateChannelMode(channelName, mode, intent);
                    strMode += 't';
                    break;
                }
                case Channel::Mode::OP_PRIVILEGE:
                {
                    if (cmd.params.size() < (j + 1))
                        continue;// [TODO] handle invalid mode params
                    Client *target = getClientByNick(cmd.params[j]);
                    j++;
                    if (!target)
                    {
                        queueMessages.push(NumericReplies::noSuchUser(clients[cmd.client], cmd.params[j - 1]));
                        continue;
                    }
                    if (!channels.isMemberOfChannel(channelName, target->getSocket()))
                    {
                        queueMessages.push(NumericReplies::userNotInChannel(channelName, clients[cmd.client], *target));
                        continue;
                    }
                    channel->updateOperators(target->getSocket(), intent == '+');
                    strMode += 'o';
                    params += " " + cmd.params[j - 1];
                    break;
                }
                default:
                    break;
            }
        }
        if (!strMode.empty())
        {
            strModes += intent + strMode;
            shouldBroadcastModeChange = true;
        }
    }
    if (shouldBroadcastModeChange)
        channels.broadcastModeChange(clients[cmd.client], channelName, strModes + params);
}

void IrcServer::HandleJoinCmd(const IrcCommand::JoinCmd &cmd)
{
    Client &client = clients[cmd.client];
    if (!client.isAuthenticated())
        return;
    for(size_t i = 0; i < cmd.channels.size(); i++)
    {
        std::string channelName = cmd.channels[i].substr(1);
        Channel *channel = channels.getChannel(channelName);
        if (channel && channel->isMember(cmd.client))
            continue;
        if (channel && channel->modeIsSet(Channel::Mode::INVITE_ONLY) && !channel->isInvited(client.getNick()))
        {
            queueMessages.push(NumericReplies::isInviteOnly(channelName, clients[cmd.client]));
            continue;
        }
        if (channel && channel->modeIsSet(Channel::Mode::REQUIRE_PASS) && (cmd.keys.size() <= i || !channel->isValidKey(cmd.keys[i])))
        {
            queueMessages.push(NumericReplies::invalidChannelKey(channelName, clients[cmd.client]));
            continue;
        }
        if (channel && channel->modeIsSet(Channel::Mode::USER_LIMIT) && channel->isFull())
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
        Logger::info("Client: " + client.info() + " has joined " + channelName);
    }
}

void IrcServer::HandleNickCmd(const IrcCommand::NickCmd &cmd)
{
    Client *client = getClientByNick(cmd.nickname);
    if (client)
    {
        queueMessages.push(NumericReplies::nickInUse(cmd.nickname, clients[cmd.client]));
        return;
    }
    else
    {
        client = &clients[cmd.client];
    }
    client->setNick(cmd.nickname);
    client->updateHandshakeState(Client::Handshake::RECEIVED_NICK);
    if (!client->isAuthenticated() && client->readyToAuthenticate())
    {
        authenticate(*client);
    }
}

void IrcServer::HandlePassCmd(const IrcCommand::PassCmd &cmd)
{
    Client &client = clients[cmd.client];
    client.setPass(cmd.password);
    client.updateHandshakeState(Client::Handshake::RECEIVED_PASS);
    if (!client.isAuthenticated() && client.readyToAuthenticate())
    {
        authenticate(client);
    }
}

void IrcServer::HandleCapCmd(const IrcCommand::CapCmd &cmd)
{
   Logger::debug("Ignoring capability handshake");
   clients[cmd.client].updateHandshakeState(Client::Handshake::RECEIVED_CAP);
}

void IrcServer::HandleUserCmd(const IrcCommand::UserCmd &cmd)
{
    Client &client = clients[cmd.client];
    client.setFullname(cmd.fullName);
    client.setUsername(cmd.user);
    client.updateHandshakeState(Client::Handshake::RECEIVED_USER);
    if (!client.isAuthenticated() && client.readyToAuthenticate())
    {
        authenticate(client);
    }
}

void IrcServer::HandleTopicCmd(const IrcCommand::TopicCmd &cmd)
{
    Client &client = clients[cmd.client];
    if (!client.isAuthenticated())
        return;
    std::string channelName = cmd.channel.substr(cmd.channel.find('#') + 1);
    Channel *channel = channels.getChannel(channelName);
    if (!channel)
    {
        queueMessages.push(NumericReplies::channelNotFound(channelName, clients[cmd.client]));
        return;
    }
    if (!channel->isMember(cmd.client))
    {
        queueMessages.push(NumericReplies::notChannelMember(channelName, clients[cmd.client]));
        return;
    }
    // TOPIC with no colon present - query mode
    if (!cmd.topicProvided)
    {
        const std::string &topic = channel->getTopic();
        if (topic.empty())
        {
            queueMessages.push({cmd.client, NumericReplies::noTopicReply(channelName, clients[cmd.client].getNick())});
        }
        else
        {
            queueMessages.push({cmd.client, NumericReplies::topicReply(channelName, clients[cmd.client].getNick(), topic)});
            queueMessages.push({cmd.client, NumericReplies::topicSetBy(channelName, clients[cmd.client].getNick(), channel->getTopicSetter(), channel->getTopicTime())});
        }
        return;
    }
    // If +t mode is set, only ops can change the topic
    if (channel->modeIsSet(Channel::Mode::RESTRICT_TOPIC) && !channel->isOperator(cmd.client))
    {
        queueMessages.push({cmd.client, NumericReplies::makeBody(482, clients[cmd.client].getNick(), channelName, "You're not channel operator")});
        return;
    }
    // Set or clear the topic
    channel->setTopic(cmd.topic, clients[cmd.client].getNick());
    BroadcastMessage broadcast;
    std::string msg = ":" + clients[cmd.client].getNick() + " TOPIC #" + channelName + " :" + cmd.topic + "\r\n";
    broadcast.msg = msg;
    const std::vector<int> &members = channel->getClients();
    broadcast.clientFds = members;
    broadcast.totalSent = 0;
    queueBroadcastMessages.push(broadcast);
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
