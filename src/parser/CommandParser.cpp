#include "CommandParser.hpp"
#include "commands/IrcCommand.hpp"
#include "commands/RawIrcCommand.hpp"
#include "utils/Logger.hpp"
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

static std::optional<IrcCommand> TryParseNick(RawIrcCommand const& raw)
{
    if (raw.cmd.starts_with("NICK"))
    {
        Logger::info("translating NICK");
        IrcCommand::NickCmd nick;
        nick.nickname = raw.cmd.substr(5);
        nick.client = raw.client;
        return IrcCommand(nick);
    }
    return std::nullopt;
}

// static std::optional<IrcCommand> TryParseUser(RawIrcCommand const& raw)
// {
//     if (raw.cmd.starts_with("USER"))
//     {
//         Logger::info("Nick: " + raw.cmd);
//         IrcCommand::UserCmd user;
//         user.client = raw.client;
//         return IrcCommand(user);
//     }
//     return std::nullopt;
// }
static std::optional<IrcCommand> TryParsePass(RawIrcCommand const& raw)
{
    if (raw.cmd.starts_with("PASS"))
    {
        IrcCommand::PassCmd pass;
        pass.password = raw.cmd.substr(5);
        return IrcCommand(pass);
    }
    return std::nullopt;
}
static std::optional<IrcCommand> TryParseJoin(RawIrcCommand const& raw)
{
    if (raw.cmd.starts_with("JOIN"))
    {
        IrcCommand::JoinCmd join;
        join.channels = raw.cmd.substr(5);
        return IrcCommand(join);
    }
    return std::nullopt;
}

static std::optional<IrcCommand> TryParsePrivMsg(RawIrcCommand const& raw)
{
    if (raw.cmd.starts_with("PRIVMSG"))
    {
        IrcCommand::PrivMsgCmd msg;
        size_t start = 8; // after "PRIVMSG "
        size_t end = raw.cmd.find(' ', start);
        msg.targets = raw.cmd.substr(start, end - start);
        msg.say_text = raw.cmd.substr(raw.cmd.find(':') + 1);
        Logger::info(raw.cmd);
        Logger::info("Message: " + msg.say_text + " target: " + msg.targets);
        return IrcCommand(msg);
    }
    return std::nullopt;
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
            Logger::info("Successfully parsed a PASS irc command.\r\n");
            return cmd;
        }
    }
    {
        std::optional<IrcCommand> cmd = TryParsePass(raw);
        if (cmd.has_value())
        {
            std::cerr << "Successfully parsed a PASS irc command.\r\n";
            return cmd;
        }
    }
    {
        std::optional<IrcCommand> cmd = TryParseJoin(raw);
        if (cmd.has_value())
        {
            std::cerr << "Successfully parsed a Join irc command.\r\n";
            return cmd;
        }
    }
    {
        std::optional<IrcCommand> cmd = TryParsePrivMsg(raw);
        if (cmd.has_value())
        {
            std::cerr << "Successfully parsed a Join privMsg command.\r\n";
            return cmd;
        }
    }
    return std::nullopt;
}
