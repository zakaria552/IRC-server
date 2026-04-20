#pragma once
#include <string>
#include <unordered_map>
#include "Channel.hpp"
#include "server/Client.hpp"

using Channels = std::unordered_map<std::string, Channel>;

class ChannelsManager
{
    Channels channels;
public:
    void add(const std::string &channel, int clientFd);
    void sendMessage(const Client &sender , const std::string &targets, const std::string &msg);
    bool channelExist(const std::string &channel);
    bool isMemberOfChannel(const std::string &channel, int clientFd);
};
