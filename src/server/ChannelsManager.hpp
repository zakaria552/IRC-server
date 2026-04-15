#pragma once
#include <string>
#include <unordered_map>
#include "Channel.hpp"

using Channels = std::unordered_map<std::string, Channel>;

class ChannelsManager
{
    Channels channels;
public:
    void add(const std::string &channel, const unsigned int &clientId);
};
