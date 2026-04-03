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
};

struct BaseCmd
{
    int client;
};

struct CapCmd : public BaseCmd
{
    int version;
};

struct PassCmd : public BaseCmd
{
    std::string password;
};

struct PrivMsgCmd : public BaseCmd
{
    std::string say_text;
};

union CmdPayload
{
    CapCmd cap;
    PassCmd pass;
    PrivMsgCmd privmsg;

    CmdPayload();
    ~CmdPayload();
    CmdPayload(CmdPayload const&);
};

#endif
