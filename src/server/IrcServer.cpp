#include "IrcServer.hpp"
#include "commands/IrcCommand.hpp"
#include "commands/IrcCommands.hpp"
#include "parser/RawCommandParser.hpp"
#include "parser/CommandParser.hpp"
#include "server/Channel.hpp"
#include "server/ChannelsManager.hpp"
#include "server/Client.hpp"
#include "server/NumericReplies.hpp"
#include "server/MessageBroker.hpp"
#include "utils/Logger.hpp"
#include <cerrno>
#include <charconv>
#include <optional>
#include <set>
#include <stdexcept>
#include <cstring>
#include <string>
#include <sys/socket.h>
#include <system_error>
#include "server/globals.hpp"

IrcServer::IrcServer(const std::string &name, const char *port, const char *password)
    : password(password), msgBroker{}, channels(msgBroker)
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

void IrcServer::setShutdownFlag(std::atomic<bool> *flag)
{
    shutdownFlag = flag;
}

void IrcServer::start()
{
    Logger::info("Starting sever");
    if (listen(socketFd, DEFAULT_BACKLOG) != 0)
        throw std::runtime_error("Failed to listen for connections on the socket" + std::string(strerror(errno)));
    while (!closeConnection)
    {
        if (shutdownFlag && shutdownFlag->load(std::memory_order_relaxed))
            break;
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
        msgBroker.flush();
    }
}

void IrcServer::shutdown()
{
    Logger::info("Shutting down server...");
    for (auto &[fd, client] : clients)
    {
        ::shutdown(fd, SHUT_RDWR);
        ::close(fd);
    }
    clients.clear();
    if (socketFd >= 0)
    {
        ::close(socketFd);
        socketFd = -1;
    }
    Logger::info("Server shutdown complete");
}

void IrcServer::newClient()
{
    struct sockaddr clientAddr;
    pollfd newPoll = {};
    socklen_t addrLen = sizeof(clientAddr);
    int clientFd;
    if ((clientFd = accept(socketFd, &clientAddr, &addrLen)) < 0)
    {
        Logger::warning("Failed to accept client: " + std::string(std::strerror(errno)));
        return;
    }

    unsigned char* ipBytePtr = reinterpret_cast<unsigned char*>(&((sockaddr_in*)&clientAddr)->sin_addr.s_addr);
    std::string ipString = std::to_string(ipBytePtr[0]) + "." + std::to_string(ipBytePtr[1]) + "." + std::to_string(ipBytePtr[2]) + "." + std::to_string(ipBytePtr[3]);
    clients[clientFd] = Client(clientFd, ((sockaddr_in*)&clientAddr)->sin_addr.s_addr);
    newPoll.fd = clientFd;
    newPoll.events = POLLIN;
    ioEvents.add(newPoll);

    Logger::info("New client from: " + ipString);
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
            case IrcCommand::PART:
                HandlePartCmd(cmds.front().payload.part);
                break;
            case IrcCommand::KICK:
                HandleKickCmd(cmds.front().payload.kick);
                break;
        }
        cmds.pop();
    }
}
// [TODO] remove stale messages
void IrcServer::clientDisconnected(int clientFd)
{
    Logger::info("Client disconnected");
    channels.leaveAll(clientFd);
    // .erase() was sometimes returning 0 instead of expected 1 element removed.
    if (clients.erase(clientFd) == 0) [[unlikely]]
    {
        Logger::error("UNEXPECTED FAIL TO ERASE CLIENT!");
        std::exit(1);
    }
    msgBroker.removeStaleMessages(clientFd);
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
        msgBroker.enqueue({client.getSocket(), body});
        clientDisconnected(client.getSocket());
        return;
    }
    std::string body = NumericReplies::welcome() + client.getNick() + " :Welcome to the Internet Relay Network " + client.getNick() + "\r\n";
    msgBroker.enqueue({client.getSocket(), body});
    client.updateHandshakeState(Client::Handshake::AUTHENTICATED);
}

// Handlers
void IrcServer::HandlePingCmd(const IrcCommand::PingCmd &cmd)
{
    Client &client = clients[cmd.client];
    if (!client.isAuthenticated())
        return;
    std::string src = ":";
    std::string body = src + serverName + " " + cmd.token + "\r\n";
    msgBroker.enqueue({cmd.client, body});
}

void IrcServer::HandlePrivMsgCmd(const IrcCommand::PrivMsgCmd &cmd)
{
    Client &client = clients[cmd.client];
    if (!client.isAuthenticated())
        return;
    for(auto &target: cmd.targets)
    {
        if (target[0] == '#')
        {
            std::string channelName = target.substr(1);
            bool onlyToOperators = channelName.starts_with('@');
            if (onlyToOperators)
                channelName = channelName.substr(1);
            Channel *channel = channels.getChannel(channelName);
            if (channel)
                msgBroker.enqueue(channel->constructMessage(client, cmd.say_text, onlyToOperators));
            continue;
        }
        Client *recepient = this->getClientByNick(target);
        if (recepient && (recepient->getSocket() != cmd.client))
        {
            std::string src = ":" + client.getNick();
            std::string body = src + " PRIVMSG " + target + " :" + cmd.say_text + "\r\n";
            msgBroker.enqueue({recepient->getSocket(), body});
            continue;
        }
    }
}

void IrcServer::HandleInviteCmd(const IrcCommand::InviteCmd &cmd)
{
    Client &client = clients[cmd.client];
    if (!client.isAuthenticated())
        return;
    Channel *channel = channels.getChannel(cmd.channel);
    if (!channel)
    {
        msgBroker.enqueue(NumericReplies::channelNotFound(cmd.channel, client));
        return;
    }
    if (!channel->isMember(cmd.client))
    {
        msgBroker.enqueue(NumericReplies::notChannelMember(cmd.channel, client));
        return;
    }
    Client *recipient = getClientByNick(cmd.nick);
    if (!recipient || (recipient->getSocket() == cmd.client))
    {
        msgBroker.enqueue(NumericReplies::noSuchUser(client, cmd.nick));
        return;
    }
    if (channel->isMember(recipient->getSocket()))
    {
        msgBroker.enqueue(NumericReplies::isChannelMember(cmd.channel, client, *recipient));
        return;
    }
    std::string src = ":" + client.getNick();
    std::string body = src + " INVITE " + cmd.nick + " :#" + cmd.channel + "\r\n";
    channel->invite(recipient->getSocket());
    msgBroker.enqueue({recipient->getSocket(), body});
    if (channel->isBlackListed(recipient->getSocket()))
        channel->removeFromBlacklist(recipient->getSocket());
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
        msgBroker.enqueue(NumericReplies::channelNotFound(channelName, client));
        return;
    }
    if (!channels.isMemberOfChannel(channelName, cmd.client))
    {
        msgBroker.enqueue(NumericReplies::notChannelMember(channelName, client));
        return;
    }
    Channel *channel = channels.getChannel(channelName);
    if (cmd.listOfModes.empty())
    {
        msgBroker.enqueue(NumericReplies::listModes(channelName, channel->listModes(), client));
        return;
    }
    if (!channel->isOperator(cmd.client))
    {
        msgBroker.enqueue(NumericReplies::isNotOperator(channelName, client));
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
                            msgBroker.enqueue(NumericReplies::invalidModeParams(channelName, client, "l " + cmd.params[j-1], "Invalid argument"));
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
                        msgBroker.enqueue(NumericReplies::noSuchUser(client, cmd.params[j - 1]));
                        continue;
                    }
                    if (!channels.isMemberOfChannel(channelName, target->getSocket()))
                    {
                        msgBroker.enqueue(NumericReplies::userNotInChannel(channelName, client, *target));
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
        channels.broadcastModeChange(client, channelName, strModes + params);
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
        if (channel && channel->isBlackListed(cmd.client))
        {
            msgBroker.enqueue(NumericReplies::bannedFromChannel(channelName, client));
            continue;
        }
        if (channel && channel->modeIsSet(Channel::Mode::INVITE_ONLY) && !channel->isInvited(cmd.client))
        {
            msgBroker.enqueue(NumericReplies::isInviteOnly(channelName, client));
            continue;
        }
        if (channel && channel->modeIsSet(Channel::Mode::REQUIRE_PASS) && (cmd.keys.size() <= i || !channel->isValidKey(cmd.keys[i])))
        {
            msgBroker.enqueue(NumericReplies::invalidChannelKey(channelName, client));
            continue;
        }
        if (channel && channel->modeIsSet(Channel::Mode::USER_LIMIT) && channel->isFull())
        {
            msgBroker.enqueue(NumericReplies::channelIsFull(channelName, client));
            continue;
        }
        if (!channel)
            channel = channels.newChannel(channelName);
        if (channel->modeIsSet(Channel::INVITE_ONLY))
            channel->removeInvite(cmd.client);
        channel->addClient(cmd.client);
        channels.broadcastJoinedUser(client, channelName);
        sendListOfUsers(client, channel);
        Logger::info("Client: " + client.info() + " has joined " + channelName);
    }
}

void IrcServer::HandleNickCmd(const IrcCommand::NickCmd &cmd)
{
    bool nickInUse = (getClientByNick(cmd.nickname) != nullptr);
    Client &client = clients[cmd.client];
    if (nickInUse)
    {
        msgBroker.enqueue(NumericReplies::nickInUse(cmd.nickname, client));
        Logger::debug("Duplicate name");
        return;
    }
    std::string body = ":" + client.getNick() + " NICK " + cmd.nickname + "\r\n";
    client.setNick(cmd.nickname);
    client.updateHandshakeState(Client::Handshake::RECEIVED_NICK);
    if (!client.isAuthenticated() && client.readyToAuthenticate())
        authenticate(client);
    msgBroker.enqueue({cmd.client, body});
    for(int fd: channels.getSharedChannelClients(cmd.client))
    {
        msgBroker.enqueue({fd, body});
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
    Logger::debug("Handling capability negotiation");
    Client &client = clients[cmd.client];
    if (client.getHandshakeState() & Client::Handshake::RECEIVED_CAP)
        return;
    if (!client.isAuthenticated() && client.readyToAuthenticate())
    {
        authenticate(client);
    }
    msgBroker.enqueue({cmd.client, ":" + serverName + " CAP * LS\r\n"});
    client.updateHandshakeState(Client::Handshake::RECEIVED_CAP);
}

void IrcServer::HandleUserCmd(const IrcCommand::UserCmd &cmd)
{
    Client &client = clients[cmd.client];
    client.setFullname(cmd.fullName);
    client.setUsername(cmd.user);
    client.setHost(cmd.host);
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
        msgBroker.enqueue(NumericReplies::channelNotFound(channelName, client));
        return;
    }
    if (!channel->isMember(cmd.client))
    {
        msgBroker.enqueue(NumericReplies::notChannelMember(channelName, client));
        return;
    }
    // TOPIC with no colon present - query mode
    if (!cmd.topicProvided)
    {
        const std::string &topic = channel->getTopic();
        if (topic.empty())
        {
           msgBroker.enqueue({cmd.client, NumericReplies::noTopicReply(channelName, client.getNick())});
        }
        else
        {
           msgBroker.enqueue({cmd.client, NumericReplies::topicReply(channelName, client.getNick(), topic)});
           msgBroker.enqueue({cmd.client, NumericReplies::topicSetBy(channelName, client.getNick(), channel->getTopicSetter(), channel->getTopicTime())});
        }
        return;
    }
    // If +t mode is set, only ops can change the topic
    if (channel->modeIsSet(Channel::Mode::RESTRICT_TOPIC) && !channel->isOperator(cmd.client))
    {
        msgBroker.enqueue({cmd.client, NumericReplies::makeBody(482, client.getNick(), channelName, "You're not channel operator")});
        return;
    }
    // Set or clear the topic
    channel->setTopic(cmd.topic, client.getNick());
    MessageBroker::BroadcastMessage broadcast;
    std::string msg = ":" + client.prefix() + " TOPIC #" + channelName + " :" + cmd.topic + "\r\n";
    broadcast.msg = msg;
    broadcast.clientFds = channel->getClients();
    msgBroker.enqueue(broadcast);
}

void IrcServer::HandlePartCmd(const IrcCommand::PartCmd &cmd)
{
    Client &client = clients[cmd.client];
    if (!client.isAuthenticated())
        return;
    for(const std::string &chan: cmd.channels)
    {
        if (!chan.starts_with('#'))
            continue;
        std::string channelName = chan.substr(1);
        Channel *channel = channels.getChannel(channelName);
        if (!channel)
        {
            msgBroker.enqueue(NumericReplies::channelNotFound(channelName, client));
            continue;
        }
        if (!channel->isMember(cmd.client))
        {
            msgBroker.enqueue(NumericReplies::notChannelMember(channelName, client));
            continue;
        }
        MessageBroker::BroadcastMessage broadcast;
        broadcast.msg = ":" + client.prefix() + " PART #" + channelName;
        if (!cmd.reason.empty())
            broadcast.msg += " :" + cmd.reason;
        broadcast.msg += "\r\n";
        broadcast.clientFds = channel->getClients();
        msgBroker.enqueue(broadcast);
        channel = channels.leaveChannel(channelName, cmd.client);
        if (channel && !channel->hasOperator())
        {
            Client &clientToPromote = clients[channel->getOldestClient()];
            std::string strModes = "MODE " + chan + " +o " + clientToPromote.getNick();
            channels.broadcastModeChange(clientToPromote, channelName, strModes);
            channel->updateOperators(clientToPromote.getSocket(), true);
        }
    }
}

void IrcServer::HandleKickCmd(const IrcCommand::KickCmd &cmd)
{
    Client &client = clients[cmd.client];
    if (!client.isAuthenticated())
        return;
    std::string channelName = cmd.channel.substr(1);
    Channel *channel = channels.getChannel(channelName);
    if (!channel)
    {
        msgBroker.enqueue(NumericReplies::channelNotFound(channelName, client));
        return;
    }
    if (!channel->isMember(cmd.client))
    {
        msgBroker.enqueue(NumericReplies::notChannelMember(channelName, client));
        return;
    }
    if (!channel->isOperator(cmd.client))
    {
        msgBroker.enqueue(NumericReplies::isNotOperator(channelName, client));
        return;
    }
    for(const std::string &nick: cmd.targets)
    {
        Client *target = getClientByNick(nick);
        if (!channel->isMember(cmd.client))
        {
            msgBroker.enqueue(NumericReplies::notChannelMember(channelName, client));
            continue;
        }
        if (!target || (target->getSocket() == cmd.client))
        {
            msgBroker.enqueue(NumericReplies::noSuchUser(client, nick));
            continue;
        }
        MessageBroker::BroadcastMessage broadcast;
        broadcast.msg = ":" + client.prefix() + " KICK #" + channelName + " " + nick;
        if (!cmd.reason.empty())
            broadcast.msg += " :" + cmd.reason;
        broadcast.msg += "\r\n";
        broadcast.clientFds = channel->getClients();
        msgBroker.enqueue(broadcast);
        channel->kickClient(target->getSocket());
    }
}

void IrcServer::sendListOfUsers(const Client &client, Channel *channel)
{
    MessageBroker::Message message; //"<client> <symbol> <channel> :[prefix]<nick>{ [prefix]<nick>}"
    const std::string body = ":" + serverName + " 353 " + client.prefix() + " =" + " #" + channel->getName() + " :";
    const std::set<int> users = channel->getClients();
    message.clientFd = client.getSocket();
    for(int clientFd: users)
    {
        Client &member = clients[clientFd];
        std::string opPrefix = channel->isOperator(member.getSocket()) ? "@" : "";
        message.msg = body + opPrefix + member.prefix() + "\r\n";
        msgBroker.enqueue(message);
    }
    //:server 366 alice #chat :End of /NAMES list
    message.msg = ":" + serverName + " 366 " + client.prefix() + " #" + channel->getName() + " :End of /NAMES list\r\n";
    msgBroker.enqueue(message);
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
