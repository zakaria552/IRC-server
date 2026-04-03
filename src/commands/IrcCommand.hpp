#ifndef _IRC_COMMAND_HPP_
#define _IRC_COMMAND_HPP_

#include <string>

struct IrcCommand
{
   #include "IrcCommands.hpp"

    Type type;
    CmdPayload payload;

    IrcCommand();
    ~IrcCommand();
    IrcCommand(IrcCommand const&) = default;
};

#endif
