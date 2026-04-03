#ifndef _COMMAND_PARSER_HPP_
#define _COMMAND_PARSER_HPP_

#include "commands/IrcCommand.hpp"
#include "commands/RawIrcCommand.hpp"
#include <optional>

// Translates `RawIrcCommand`s into `IrcCommand`s
// This class could very well be a standalone function.
class CommandParser
{
public:
    CommandParser(void);
    ~CommandParser(void);

    CommandParser(CommandParser const&) = delete;
    auto operator=(CommandParser const&) -> CommandParser& = delete;

    auto Parse(RawIrcCommand const& raw) -> std::optional<IrcCommand>;
};

#endif
