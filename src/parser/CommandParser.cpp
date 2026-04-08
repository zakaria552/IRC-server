#include "CommandParser.hpp"
#include "commands/IrcCommand.hpp"
#include "commands/RawIrcCommand.hpp"
#include <iostream>
#include <optional>

CommandParser::CommandParser()
{
}

CommandParser::~CommandParser()
{
}

static
std::optional<IrcCommand> TryParseCap(RawIrcCommand const& raw)
{
    if (raw.cmd.starts_with("CAP"))
    {
        IrcCommand::CapCmd cap;
        cap.version = 0;
        cap.client = 0;
        return IrcCommand(cap);
    }

    return std::nullopt;
}

static
std::optional<IrcCommand> TryParseNick(RawIrcCommand const& raw)
{
}

std::optional<IrcCommand> CommandParser::Parse(RawIrcCommand const& raw)
{
    {
        std::optional<IrcCommand> cmd = TryParseCap(raw);
        if (cmd.has_value())
        {
            std::cerr << "Successfully parsed a CAP irc command.\r\n";
            return cmd;
        }
    }
    {
        std::optional<IrcCommand> cmd = TryParseNick(raw);
        if (cmd.has_value())
        {
            return cmd;
        }
    }
    return std::nullopt;
}
