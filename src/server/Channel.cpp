#include "Channel.hpp"
#include "server/Client.hpp"
#include "server/MessageBroker.hpp"
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

void Channel::setTopic(const std::string &topic)
{
    this->topic = topic;
}

void Channel::setKey(const std::string &key)
{
    this->key = key;
}

const std::string Channel::getKey() const
{
    return key;
};

void Channel::setMaxUserLimit(unsigned int max)
{
    this->maxUsers = max;
}

bool Channel::isValidKey(const std::string &key)
{
   return this->key == key;
}

void Channel::addClient(int clientId)
{
    operators[clientId] = (clients.size() == 0);
    clients.push_back(clientId);
}

void Channel::removeClient(int clientId)
{
    auto it = std::find(clients.begin(), clients.end(), clientId);
    if (it != clients.end())
    {
        clients.erase(it);
        operators[clientId] = false;
    }
}

MessageBroker::BroadcastMessage Channel::constructMessage(const Client &sender, const std::string &msg)
{
    MessageBroker::BroadcastMessage msgQueue;
    std::string src = ":" + sender.getNick();
    std::string body = src + " PRIVMSG #" + name + " :" + msg + "\r\n";
    msgQueue.msg = body;
    for(auto client: clients)
    {
        if (client == sender.getSocket())
            continue;
        msgQueue.clientFds.push_back(client);
        Logger::debug("Sent message to client: " + std::to_string(client) + " , " + body);
    }
    return msgQueue;
}
//      0101 & 1
bool Channel::modeIsSet(Mode mode) const
{
    return modes & mode;
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

void Channel::updateOperators(int clientFd, bool isOperator)
{
    operators[clientFd] = isOperator;
}

bool Channel::isOperator(int clientFd)
{
    return operators[clientFd];
}

std::string Channel::listModes() const
{
    std::string modes = "+";
    std::string modeArgs = "";
    if (modeIsSet(INVITE_ONLY))
        modes += "i";
    if (modeIsSet(RESTRICT_TOPIC))
        modes += "t";
    if (modeIsSet(REQUIRE_PASS))
    {
        modes += "k";
        modeArgs += " " + key;
    }
    if (modeIsSet(USER_LIMIT))
    {
        modes += "l";
        modeArgs += " " + std::to_string(maxUsers);
    }
    return modes + modeArgs;
}

bool Channel::isFull()
{
    return modeIsSet(USER_LIMIT) && clients.size() >= maxUsers;
}

bool Channel::isEmpty()
{
    return clients.empty();
}
