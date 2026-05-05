#ifndef _IRC_COMMANDS_HPP_
#define _IRC_COMMANDS_HPP_

#include <string>
enum Type
{
    UNDEFINED,
    CAP,
    NICK,
    USER,
    PASS,
    JOIN,
    INVITE,
    PRIVMSG,
    PING,
    MODE,
    TOPIC,
};

struct BaseCmd
{
    int client;
};

struct CapCmd : public BaseCmd
{
    int version;
};

struct NickCmd : public BaseCmd
{
    std::string nickname;
};

struct UserCmd : public BaseCmd
{
    // UNDONE
    std::string user;
    std::string fullName;
};

struct PassCmd : public BaseCmd
{
    std::string password;
};

struct JoinCmd : public BaseCmd
{
    std::vector<std::string> channels;
    std::vector<std::string> keys;
};

struct PrivMsgCmd : public BaseCmd
{
    std::string say_text;
    std::string targets;
};

struct PingCmd : public BaseCmd
{
    std::string token;
};

struct InviteCmd : public BaseCmd
{
    std::string nick;
    std::string channel;
};

struct TopicCmd : public BaseCmd
{
    std::string channel;
    std::string topic;
    bool topicProvided = false;
};

struct ModeCmd : public BaseCmd
{
    struct Mode
    {
        std::vector<std::uint8_t> modes;
        char intent;
    };
    std::string target;
    std::vector<Mode> listOfModes;
    std::vector<std::string> params;
};

union CmdPayload
{
    CapCmd cap;
    NickCmd nick;
    UserCmd user;
    PassCmd pass;
    JoinCmd join;
    InviteCmd invite;
    PrivMsgCmd privmsg;
    PingCmd ping;
    ModeCmd mode;
    TopicCmd topic;

    CmdPayload();
    ~CmdPayload();
    CmdPayload(CmdPayload const&) = delete;
    CmdPayload(CmdPayload&&) = delete;
};

#endif
