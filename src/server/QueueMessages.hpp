#pragma once
#include <string>
#include <vector>

struct Message
{
    int clientFd;
    std::string msg;
};

struct BroadcastMessage
{
    std::vector<int> clientFds;
    std::string msg;
    int totalSent = 0;
};
