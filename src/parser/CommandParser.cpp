#include "CommandParser.hpp"
#include "commands/IrcCommand.hpp"
#include "commands/RawIrcCommand.hpp"
#include "server/Channel.hpp"
#include "utils/Logger.hpp"
#include <optional>
#include <vector>

CommandParser::CommandParser()
{
}

CommandParser::~CommandParser()
{
}
static std::vector<std::string> splitToTokens(const std::string &str, const char delimiter)
{
    std::vector<std::string> tokens;
    size_t start = 0;
    size_t end = str.find(delimiter);
    while (end != std::string::npos)
    {
        std::string chunk = str.substr(start, end-start);
        if (!chunk.empty())
            tokens.push_back(str.substr(start, end-start));
       start = end + 1;
       end = str.find(delimiter, start);
    }
    if (start < (str.size() - 1))
        tokens.push_back(str.substr(start));
    return tokens;
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
    if (!raw.cmd.starts_with("JOIN"))
        return std::nullopt;
    IrcCommand::JoinCmd join = {};
    std::vector<std::string> tokens = splitToTokens(raw.cmd, ' ');
    if (tokens.size() == 1)
        return std::nullopt;
    join.channels = splitToTokens(tokens[1], ',');
    if (tokens.size() >= 3)
        join.keys = splitToTokens(tokens[2], ',');
    return IrcCommand(join);
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

static std::optional<IrcCommand> TryParseInvite(RawIrcCommand const& raw)
{
    if (raw.cmd.starts_with("INVITE"))
    {
        IrcCommand::InviteCmd cmd; // INVITE jack #67
        size_t start = 7;
        size_t end = raw.cmd.find(' ', start);
        cmd.nick = raw.cmd.substr(start, end - start);
        cmd.channel = raw.cmd.substr(raw.cmd.find("#") + 1);
        return IrcCommand(cmd);
    }
    return std::nullopt;
}

static std::optional<IrcCommand> TryParseMode(RawIrcCommand const& raw)
{
    if (!raw.cmd.starts_with("MODE"))
        return std::nullopt;
    IrcCommand::ModeCmd cmd{};
    std::vector<std::string> tokens = splitToTokens(raw.cmd, ' ');
    if (tokens.size() == 1)
        return std::nullopt;
    cmd.target = tokens[1];
    if (tokens.size() == 2)
        return cmd;
    if (tokens[2][0] != '-' && tokens[2][0] != '+')
        return std::nullopt;
    cmd.params = std::vector<std::string>(tokens.begin() + 3, tokens.end());
    IrcCommand::ModeCmd::Mode mode = {{}, tokens[2][0]};
    for(size_t i = 1; i < tokens[2].size(); i++)
    {
        if (tokens[2][i] == '-' || tokens[2][i] == '+')
        {
            cmd.listOfModes.push_back(mode);
            mode.intent = tokens[2][i];
            mode.modes.clear();
            continue;
        }
        switch (tokens[2][i]) {
            case 'i':
                mode.modes.push_back(Channel::Mode::INVITE_ONLY);
                break;
            case 'k':
                mode.modes.push_back(Channel::Mode::REQUIRE_PASS);
                break;
            case 't':
                mode.modes.push_back(Channel::Mode::RESTRICT_TOPIC);
                break;
            case 'l':
                mode.modes.push_back(Channel::Mode::USER_LIMIT);
                break;
            case 'o':
                mode.modes.push_back(Channel::Mode::OP_PRIVILEGE);
                break;
            default:
                Logger::warning("Mode [" + std::to_string(tokens[2][i]) + "] is not supported");
                continue;
        }
    }
    cmd.listOfModes.push_back(mode);
    return IrcCommand(cmd);
}

static std::optional<IrcCommand> TryParseTopic(RawIrcCommand const& raw)
{
    if (!raw.cmd.starts_with("TOPIC"))
        return std::nullopt;
    // Must have at least "TOPIC " (6 chars) to contain a channel argument
    if (raw.cmd.size() <= 6 or raw.cmd[5] != ' ')
        return std::nullopt;
    IrcCommand::TopicCmd cmd;
    size_t start = 6; // after "TOPIC "
    size_t end = raw.cmd.find(' ', start);
    cmd.channel = raw.cmd.substr(start, end - start);
    if (cmd.channel.empty())
        return std::nullopt;
    size_t colonPos = raw.cmd.find(':', start + cmd.channel.size());
    if (colonPos != std::string::npos) {
        cmd.topic = raw.cmd.substr(colonPos + 1);
        cmd.topicProvided = true;
    }
    return IrcCommand(cmd);
}

std::optional<IrcCommand> CommandParser::Parse(RawIrcCommand const& raw, int clientFd)
{
    Logger::debug(raw.cmd);
    {
        std::optional<IrcCommand> cmd = TryParseCap(raw);
        if (cmd.has_value())
        {
            cmd.value().payload.cap.client = clientFd;
            return cmd;
        }
    }
    {
        std::optional<IrcCommand> cmd = TryParseUser(raw);
        if (cmd.has_value())
        {
            cmd.value().payload.user.client = clientFd;
            return cmd;
        }
    }
    {
        std::optional<IrcCommand> cmd = TryParseNick(raw);
        if (cmd.has_value())
        {
            cmd.value().payload.nick.client = clientFd;
            return cmd;
        }
    }
    {
        std::optional<IrcCommand> cmd = TryParsePass(raw);
        if (cmd.has_value())
        {
            cmd.value().payload.pass.client = clientFd;
            return cmd;
        }
    }
    {
        std::optional<IrcCommand> cmd = TryParseJoin(raw);
        if (cmd.has_value())
        {
            cmd.value().payload.join.client = clientFd;
            return cmd;
        }
    }
    {
        std::optional<IrcCommand> cmd = TryParsePrivMsg(raw);
        if (cmd.has_value())
        {
            cmd.value().payload.privmsg.client = clientFd;
            return cmd;
        }
    }
    {
        std::optional<IrcCommand> cmd = TryParsePing(raw);
        if (cmd.has_value())
        {
            cmd.value().payload.ping.client = clientFd;
            return cmd;
        }
    }
    {
        std::optional<IrcCommand> cmd = TryParseInvite(raw);
        if (cmd.has_value())
        {
            cmd.value().payload.invite.client = clientFd;
            return cmd;
        }
    }
    {
        std::optional<IrcCommand> cmd = TryParseMode(raw);
        if (cmd.has_value())
        {
            cmd.value().payload.mode.client = clientFd;
            return cmd;
        }
    }
    {
        std::optional<IrcCommand> cmd = TryParseTopic(raw);
        if (cmd.has_value())
        {
            cmd.value().payload.topic.client = clientFd;
            return cmd;
        }
    }
    return std::nullopt;
}
