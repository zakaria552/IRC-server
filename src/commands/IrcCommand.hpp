#ifndef _IRC_COMMAND_HPP_
#define _IRC_COMMAND_HPP_

#include <string>

struct IrcCommand
{
    enum Type
    {
        UNDEFINED,
        CAP,
        NICK,
        USER,
        PASS,
        JOIN
    };
    #include "IrcCommands.hpp"

    Type type;
    CmdPayload payload;
};

#endif
