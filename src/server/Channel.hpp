#pragma once
#include <string>
#include <vector>

class Channel
{
    std::string topic;
    std::vector<unsigned int> clients;
    std::vector<unsigned int> blackList;
    void *mode; // unspecified for now
public:
    Channel() = default;
    ~Channel() = default;
    bool isBlackListed(const unsigned int &clientId);
    bool isMember(const unsigned int &clientId);
    void addClient(const unsigned int &clientId);
};
