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

static std::optional<IrcCommand> TryParseUser(RawIrcCommand const& raw)
{
    if (raw.cmd.starts_with("USER"))
    {
        IrcCommand::UserCmd user;
        user.client = raw.client;
        size_t start = 5;
        size_t end = raw.cmd.find(' ', start);
        user.user = raw.cmd.substr(5, end - start);
        user.fullName = raw.cmd.substr(raw.cmd.find(':') + 1);
        return IrcCommand(user);
    }
    return std::nullopt;
}
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
        size_t start = 8;
        size_t end = raw.cmd.find(' ', start);
        msg.targets = raw.cmd.substr(start, end - start);
        msg.say_text = raw.cmd.substr(raw.cmd.find(':') + 1);
        return IrcCommand(msg);
    }
    return std::nullopt;
}

static std::optional<IrcCommand> TryParsePing(RawIrcCommand const& raw)
{
    Logger::info(raw.cmd);
    if (raw.cmd.starts_with("PING"))
    {
        IrcCommand::PingCmd cmd;
        cmd.client = raw.client;
        cmd.token = raw.cmd;
        cmd.token[1] = 'O';
        return IrcCommand(cmd);
    }
    return std::nullopt;
}


std::optional<IrcCommand> CommandParser::Parse(RawIrcCommand const& raw)
{
    Logger::info(raw.cmd);
    {
        std::optional<IrcCommand> cmd = TryParseCap(raw);
        if (cmd.has_value())
        {
            std::cerr << "Successfully parsed a CAP irc command.\r\n";
            return cmd;
        }
    }
    {
        std::optional<IrcCommand> cmd = TryParseUser(raw);
        if (cmd.has_value())
        {
            std::cerr << "Successfully parsed a User command.\r\n";
            return cmd;
        }
    }
    {
        std::optional<IrcCommand> cmd = TryParseNick(raw);
        if (cmd.has_value())
        {
            Logger::info("Successfully parsed a NICK irc command.\r\n");
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
            std::cerr << "Successfully parsed a JOIN irc command.\r\n";
            return cmd;
        }
    }
    {
        std::optional<IrcCommand> cmd = TryParsePrivMsg(raw);
        if (cmd.has_value())
        {
            std::cerr << "Successfully parsed a PRIVMSG command.\r\n";
            return cmd;
        }
    }
    {
        std::optional<IrcCommand> cmd = TryParsePing(raw);
        if (cmd.has_value())
        {
            // std::cerr << "Successfully parsed a PING command.\r\n";
            std::cerr << "PING cmd.\r\n";
            return cmd;
        }
    }
    return std::nullopt;
}
