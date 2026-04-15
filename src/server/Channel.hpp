#pragma once
#include "server/Client.hpp"
#include <string>
#include <vector>

class Channel
{
    std::string name;
    std::string topic;
    std::vector<unsigned int> clients;
    std::vector<unsigned int> blackList;
    void *mode; // unspecified for now
public:
    Channel() = default;
    Channel(const std::string &name);
    ~Channel() = default;
    bool isBlackListed(const unsigned int &clientId);
    bool isMember(const unsigned int &clientId);
    void addClient(const unsigned int &clientId);
    void sendMessage(const Client &sender, const std::string &msg);
};
