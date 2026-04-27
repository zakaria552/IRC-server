#ifndef _IRC_COMMANDS_HPP_
#define _IRC_COMMANDS_HPP_

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
    // UNDONE
    std::string channels;
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

struct ModeCmd : public BaseCmd
{
    std::string channel;
    std::string target;
    uint8_t mode;
    char intent;
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

    CmdPayload();
    ~CmdPayload();
    CmdPayload(CmdPayload const&) = delete;
    CmdPayload(CmdPayload&&) = delete;
};

#endif
