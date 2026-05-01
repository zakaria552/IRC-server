#pragma once
#include <queue>
#include <string>
#include <unordered_map>
#include "Channel.hpp"
#include "server/Client.hpp"
#include "server/QueueMessages.hpp"
using Channels = std::unordered_map<std::string, Channel>;

class ChannelsManager
{
    Channels channels;
    std::queue<BroadcastMessage> &queue;
public:
    ChannelsManager(std::queue<BroadcastMessage> &queues);
    void add(const std::string &channel, int clientFd);
    void sendMessage(const Client &sender , const std::string &targets, const std::string &msg);
    void broadcastModeChange(const Client &client, const std::string &channel, const std::string &rawCmd);
    void broadcastJoinedUser(const Client &client, const std::string &channel);
    bool channelExist(const std::string &channel);
    bool isMemberOfChannel(const std::string &channel, int clientFd);
    void updateChannelMode(const std::string &channel, Mode mode, char intent);
    uint8_t getChannelModes(const std::string &channel);
    Channel *getChannel(const std::string &channel);
    Channel *newChannel(const std::string &channel);
};
