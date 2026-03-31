#ifndef _IRC_COMMANDS_HPP_
#define _IRC_COMMANDS_HPP_

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

union CmdPayload
{
    CapCmd cap;
    PassCmd pass;
};

#endif
