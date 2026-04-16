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
    PRIVMSG,
    PING,
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

union CmdPayload
{
    CapCmd cap;
    NickCmd nick;
    UserCmd user;
    PassCmd pass;
    JoinCmd join;
    PrivMsgCmd privmsg;
    PingCmd ping;

    CmdPayload();
    ~CmdPayload();
    CmdPayload(CmdPayload const&) = delete;
    CmdPayload(CmdPayload&&) = delete;
};

#endif
