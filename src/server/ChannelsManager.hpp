#pragma once
#include <set>
#include <string>
#include <unordered_map>
#include "Channel.hpp"
#include "server/Client.hpp"
#include "server/MessageBroker.hpp"
using Channels = std::unordered_map<std::string, Channel>;

class ChannelsManager
{
    Channels channels;
    MessageBroker &msgBroker;
public:
    ChannelsManager(MessageBroker &broker);
    void add(const std::string &channel, int clientFd);
    void sendMessage(const Client &sender , const std::string &targets, const std::string &msg);
    void broadcastModeChange(const Client &client, const std::string &channel, const std::string &rawCmd);
    void broadcastJoinedUser(const Client &client, const std::string &channel);
    bool channelExist(const std::string &channel);
    bool isMemberOfChannel(const std::string &channel, int clientFd);
    void updateChannelMode(const std::string &channel, Channel::Mode mode, char intent);
    uint8_t getChannelModes(const std::string &channel);
    Channel *getChannel(const std::string &channel);
    Channel *newChannel(const std::string &channel);
    void leaveAll(int clientFd);
    Channel *leaveChannel(const std::string &channel, int clientFd);
    std::set<int> getSharedChannelClients(int clientFd);
};
