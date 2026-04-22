#pragma once
#include "server/Client.hpp"
#include <string>
#include <vector>

enum Mode
{
   NONE = 0,
   INVITE_ONLY = 1 << 0,
   RESTRICT_TOPIC = 1 << 1,
   REQUIRE_PASS = 1 << 2,
   RESTRICT_OP_PRIVILEGE = 1 << 3,
   USER_LIMIT  = 1 << 4,
};

class Channel
{
    std::string name;
    std::string topic;
    std::vector<int> clients;
    std::vector<int> blackList;
    std::vector<std::string> inviteList;
    uint8_t modes = INVITE_ONLY; // unspecified for now
    unsigned int maxUsers;
public:
    Channel() = default;
    Channel(const std::string &name);
    ~Channel() = default;
    bool isBlackListed(int clientId);
    bool isMember(int clientId);
    void invite(const std::string &);
    bool isInvited(const std::string &);
    void removeInvite(const std::string &);
    void addClient(int clientId);
    void sendMessage(const Client &sender, const std::string &msg);
    uint8_t getModes();
    bool modeIsSet(Mode mode);
    void setMode(Mode mode);
    void unsetMode(Mode mode);
    const std::string &getName() const;
};
