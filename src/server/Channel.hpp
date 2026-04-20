#pragma once
#include "server/Client.hpp"
#include <string>
#include <vector>

class Channel
{
    std::string name;
    std::string topic;
    std::vector<int> clients;
    std::vector<int> blackList;
    //void *mode; // unspecified for now
public:
    Channel() = default;
    Channel(const std::string &name);
    ~Channel() = default;
    bool isBlackListed(int clientId);
    bool isMember(int clientId);
    void addClient(int clientId);
    void sendMessage(const Client &sender, const std::string &msg);
};
