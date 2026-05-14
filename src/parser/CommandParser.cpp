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
    if (start <= (str.size() - 1))
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
    if (!raw.cmd.starts_with("NICK"))
        return std::nullopt;
    IrcCommand::NickCmd nick;
    std::vector<std::string> tokens = splitToTokens(raw.cmd, ' ');
    if (tokens.size() < 2)
        return std::nullopt;
    nick.nickname = tokens[1];
    nick.client = raw.client;
    return IrcCommand(nick);
}
// USER <username> 0 * <realname>
// e.g: USER zfarah zfarah localhost :Zakaria Farah
static std::optional<IrcCommand> TryParseUser(RawIrcCommand const& raw)
{
    if (!raw.cmd.starts_with("USER"))
        return std::nullopt;
    IrcCommand::UserCmd user;
    std::vector<std::string> tokens = splitToTokens(raw.cmd, ':');
    if (tokens.size() < 2)
        return std::nullopt;
    user.fullName = tokens[1];
    tokens = splitToTokens(tokens[0], ' ');
    if (tokens.size() < 4)
        return std::nullopt;
    user.user = tokens[1];
    user.client = raw.client;
    user.host = tokens[3];
    return IrcCommand(user);
}
// PASS <password>
static std::optional<IrcCommand> TryParsePass(RawIrcCommand const& raw)
{
    if (!raw.cmd.starts_with("PASS"))
        return std::nullopt;
    IrcCommand::PassCmd pass;
    std::vector<std::string> tokens = splitToTokens(raw.cmd, ' ');
    if (tokens.size() < 2)
        return std::nullopt;
    pass.password = tokens[1];
    return IrcCommand(pass);
}
// JOIN <channel>{,<channel>} [<key>{,<key>}]
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

// PRIVMSG <target>{,<target>} <text to be sent>
static std::optional<IrcCommand> TryParsePrivMsg(RawIrcCommand const& raw)
{
    if (!raw.cmd.starts_with("PRIVMSG"))
        return std::nullopt;
    IrcCommand::PrivMsgCmd msg;
    std::vector<std::string> tokens = splitToTokens(raw.cmd, ':');
    if (tokens.size() < 2)
        return std::nullopt;
    msg.say_text = tokens[1];
    tokens = splitToTokens(tokens[0], ' ');
    if (tokens.size() < 2)
        return std::nullopt;
    msg.targets = splitToTokens(tokens[1], ',');
    if (msg.targets.empty())
        return std::nullopt;
    return IrcCommand(msg);
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
// INVITE <nickname> <channel>
static std::optional<IrcCommand> TryParseInvite(RawIrcCommand const& raw)
{
    if (!raw.cmd.starts_with("INVITE"))
        return std::nullopt;
    IrcCommand::InviteCmd cmd; // INVITE jack #67
    std::vector<std::string> tokens = splitToTokens(raw.cmd, ' ');
    if (tokens.size() < 3 || !tokens[2].starts_with('#'))
        return std::nullopt;
    cmd.nick = tokens[1];
    cmd.channel = tokens[2].substr(1);
    return IrcCommand(cmd);
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
// PART <channel>{,<channel>} [<reason>]
static std::optional<IrcCommand> TryParsePart(RawIrcCommand const& raw)
{
    if (!raw.cmd.starts_with("PART"))
        return std::nullopt;
    IrcCommand::PartCmd cmd = {};
    std::vector<std::string> tokens = splitToTokens(raw.cmd, ':');
    if (tokens.size() > 1)
        cmd.reason = tokens[1];
    tokens = splitToTokens(tokens[0], ' ');
    if (tokens.size() < 2)
        return std::nullopt;
    cmd.channels = splitToTokens(tokens[1], ',');
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
    {
        std::optional<IrcCommand> cmd = TryParsePart(raw);
        if (cmd.has_value())
        {
            cmd.value().payload.part.client = clientFd;
            return cmd;
        }
    }
    return std::nullopt;
}
