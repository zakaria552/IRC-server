#pragma once
#include "server/Client.hpp"
#include <cstdint>
#include <set>
#include "server/MessageBroker.hpp"

class Channel
{
public:
    enum Mode: uint8_t
    {
       NONE = 0,
       INVITE_ONLY = 1 << 0,
       RESTRICT_TOPIC = 1 << 1,
       REQUIRE_PASS = 1 << 2,
       OP_PRIVILEGE = 1 << 3,
       USER_LIMIT  = 1 << 4,
    };
private:
    std::string name = {};
    std::string topic = {};
    std::string topicSetter;
    std::string topicTime;
    std::string key = {};
    std::set<int> clients = {};
    std::set<int> blackList = {};
    std::set<int> inviteList = {};
    std::set<int> operators = {};
    uint8_t modes = NONE;
    [[maybe_unused]] unsigned int maxUsers;
public:
    Channel() = default;
    Channel(const std::string &name);
    ~Channel() = default;
    bool isBlackListed(int clientFd);
    bool isMember(int clientFd);
    void blacklist(int clientFd);
    void removeFromBlacklist(int clientFd);
    void setTopic(const std::string &topic);
    void setKey(const std::string &key);
    const std::string getKey() const ;
    void setMaxUserLimit(unsigned int max);
    bool isValidKey(const std::string &key);
    void invite(int clientFd);
    bool isInvited(int clientFd);
    void removeInvite(int clientFd);
    void addClient(int clientId);
    void removeClient(int clientId);
    void kickClient(int clientId);
    MessageBroker::BroadcastMessage constructMessage(const Client &sender, const std::string &msg, bool onlyOperators = false);
    uint8_t getModes();
    const std::set<int> &getClients() const ;
    bool modeIsSet(Mode mode) const;
    void setMode(Mode mode);
    void unsetMode(Mode mode);
    const std::string &getName() const;
    const std::string &getTopic() const;
    void setTopic(const std::string &topic, const std::string &setter = "");
    const std::string &getTopicSetter() const;
    const std::string &getTopicTime() const;
    bool hasTopic() const;
    void updateOperators(int clientFd, bool isOperator);
    bool isOperator(int clientFd);
    std::string listModes() const;
    bool isFull();
    bool isEmpty();
    int size() const;
    bool hasOperator();
    int getOldestClient();
};
