#pragma once
#include "Channel.hpp"
#include <algorithm>

bool Channel::isBlackListed(const unsigned int &clientId)
{
    return find(blackList.begin(), blackList.end(), clientId) == blackList.end();
}

bool Channel::isMember(const unsigned int &clientId)
{
    return find(clients.begin(), clients.end(), clientId) == clients.end();
}

void Channel::addClient(const unsigned int &clientId)
{
    clients.push_back(clientId);
}
