#ifndef _IRC_COMMAND_HPP_
#define _IRC_COMMAND_HPP_

#include <string>
#include <vector>

/*
 * IrcCommand is a complicated data structure because it holds non-trivial data members
 * in a union, requiring manual resource management of the payload's contents.
 */

struct IrcCommand
{
   #include "IrcCommands.hpp"

    Type type;
    CmdPayload payload;

    IrcCommand();
    ~IrcCommand();

    IrcCommand(CapCmd);
    IrcCommand(NickCmd);
    IrcCommand(UserCmd);
    IrcCommand(PassCmd);
    IrcCommand(JoinCmd);
    IrcCommand(PrivMsgCmd);
    IrcCommand(PingCmd);
    IrcCommand(InviteCmd);
    IrcCommand(ModeCmd);

    IrcCommand(IrcCommand&&) noexcept;

    IrcCommand(IrcCommand const&) = delete;
    IrcCommand& operator=(IrcCommand const&) = delete;
};

#endif
