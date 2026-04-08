#include "IrcCommand.hpp"
#include <cstring>
#include <string> // required by IrcCommands.hpp
#include "IrcCommands.hpp"

IrcCommand::IrcCommand()
: type(UNDEFINED)
{
}

IrcCommand::~IrcCommand(void)
{
    switch (type)
    {
        case IrcCommand::Type::UNDEFINED:
            // Requires no action.
            break;
        case IrcCommand::Type::CAP:
            payload.cap.~CapCmd();
            break;
        case IrcCommand::Type::NICK:
            payload.nick.~NickCmd();
            break;
        case IrcCommand::Type::USER:
            payload.user.~UserCmd();
            break;
        case IrcCommand::Type::PASS:
            payload.pass.~PassCmd();
            break;
        case IrcCommand::Type::JOIN:
            payload.join.~JoinCmd();
            break;
        case IrcCommand::Type::PRIVMSG:
            payload.privmsg.~PrivMsgCmd();
            break;
    }
}

IrcCommand::IrcCommand(IrcCommand&& other) noexcept
: type(other.type)
{
    switch (type)
    {
        case UNDEFINED:
            // Requires no action.
            break;
        case CAP:
            new (&payload.cap) CapCmd(std::move(other.payload.cap));
            break;
        case NICK:
            new (&payload.nick) NickCmd(std::move(other.payload.nick));
            break;
        case USER:
            new (&payload.user) UserCmd(std::move(other.payload.user));
            break;
        case PASS:
            new (&payload.pass) PassCmd(std::move(other.payload.pass));
            break;
        case JOIN:
            new (&payload.join) JoinCmd(std::move(other.payload.join));
            break;
        case PRIVMSG:
            new (&payload.privmsg) PrivMsgCmd(std::move(other.payload.privmsg));
            break;
    }
    other.type = Type::UNDEFINED;
}

IrcCommand::IrcCommand(CapCmd cmd)
: type(CAP)
{
    new (&payload.cap) CapCmd(std::move(cmd));
}

IrcCommand::IrcCommand(NickCmd cmd)
: type(NICK)
{
    new (&payload.nick) NickCmd(std::move(cmd));
}

IrcCommand::IrcCommand(UserCmd cmd)
: type(USER)
{
    new (&payload.user) UserCmd(std::move(cmd));
}

IrcCommand::IrcCommand(PassCmd cmd)
: type(PASS)
{
    new (&payload.pass) PassCmd(std::move(cmd));
}

IrcCommand::IrcCommand(JoinCmd cmd)
: type(JOIN)
{
    new (&payload.join) JoinCmd(std::move(cmd));
}

IrcCommand::IrcCommand(PrivMsgCmd cmd)
: type(PRIVMSG)
{
    new (&payload.privmsg) PrivMsgCmd(std::move(cmd));
}

/* CmdPayload definitions. */
/* ctor and dtor of CmdPayload intentionally do nothing as it's resources are managed by IrcCommand instead. */

IrcCommand::CmdPayload::CmdPayload()
{
}

IrcCommand::CmdPayload::~CmdPayload()
{
}
