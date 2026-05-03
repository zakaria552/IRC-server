#include "Channel.hpp"
#include "server/Client.hpp"
#include "utils/Logger.hpp"
#include <algorithm>
#include <string>
#include <ctime>

Channel::Channel(const std::string &name) : name(name){};

bool Channel::isBlackListed(int clientId)
{
    return std::find(blackList.begin(), blackList.end(), clientId) != blackList.end();
}

bool Channel::isMember(int clientId)
{
    return std::find(clients.begin(), clients.end(), clientId) != clients.end();
}

void Channel::addClient(int clientId)
{
    clients.push_back(clientId);
}

BroadcastMessage Channel::constructMessage(const Client &sender, const std::string &msg)
{
    BroadcastMessage msgQueue;
    std::string src = ":" + sender.getNick();
    std::string body = src + " PRIVMSG #" + name + " :" + msg + "\r\n";
    msgQueue.msg = body;
    for(auto client: clients)
    {
        if (client == sender.getSocket())
            continue;
        msgQueue.clientFds.push_back(client);
        Logger::info("Sent message to client: " + std::to_string(client) + " , " + body);
    }
    return msgQueue;
}

bool Channel::modeIsSet(Mode mode)
{
    return (modes >> mode) & 1;
}
void Channel::setMode(Mode mode)
{
    modes |= mode;
}
void Channel::unsetMode(Mode mode)
{
    //       111     010
    modes = modes & ~mode;
}

const std::string &Channel::getName() const
{
   return name;
}

uint8_t Channel::getModes()
{
    return modes;
}

void Channel::invite(const std::string &user)
{
    inviteList.push_back(user);
}

bool Channel::isInvited(const std::string &user)
{
    return std::find(inviteList.begin(), inviteList.end(), user) != inviteList.end();
}

void Channel::removeInvite(const std::string &user)
{
    auto it = std::find(inviteList.begin(), inviteList.end(), user);
    if (it != inviteList.end())
        inviteList.erase(it);
}
const std::string &Channel::getTopic() const
{
    return topic;
}

void Channel::setTopic(const std::string &topic, const std::string &setter)
{
    this->topic = topic;
    if (topic.empty())
    {
        topicSetter.clear();
        topicTime.clear();
    }
    else
    {
        topicSetter = setter;
        std::time_t now = std::time(nullptr);
        topicTime = std::to_string(now);
    }
}

const std::string &Channel::getTopicSetter() const
{
    return topicSetter;
}

const std::string &Channel::getTopicTime() const
{
    return topicTime;
}

bool Channel::hasTopic() const
{
    return !topic.empty();
}

const std::vector<int> &Channel::getClients() const
{
    return clients;
}
