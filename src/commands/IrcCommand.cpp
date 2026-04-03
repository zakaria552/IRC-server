#include "IrcCommand.hpp"
#include <string> // required by IrcCommands.hpp
#include "IrcCommands.hpp"

IrcCommand::IrcCommand(void)
{
}

IrcCommand::~IrcCommand(void)
{
}

IrcCommand::CmdPayload::CmdPayload()
{
}

IrcCommand::CmdPayload::~CmdPayload()
{
}

IrcCommand::CmdPayload::CmdPayload(CmdPayload const& other)
: type(other.type), payload(other.payload)
{
}
