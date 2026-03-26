#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

/// A single REPL command with its handler.
struct ReplCommand {
    std::string name;
    std::string description;
    std::function<void(const std::vector<std::string>& args)> handler;
};

/// Registry of REPL commands. Dispatches input lines to handlers.
class ReplCommandRegistry {
  public:
    void add(ReplCommand cmd) {
        commands_[cmd.name] = std::move(cmd);
    }

    /// Try to dispatch a line. Returns false if the command was not found.
    bool dispatch(const std::string& line) const {
        auto tokens = tokenize(line);
        if (tokens.empty())
            return true; // blank line, not an error

        auto it = commands_.find(tokens[0]);
        if (it == commands_.end())
            return false;

        std::vector<std::string> args(tokens.begin() + 1, tokens.end());
        it->second.handler(args);
        return true;
    }

    const std::unordered_map<std::string, ReplCommand>& commands() const {
        return commands_;
    }

  private:
    static std::vector<std::string> tokenize(const std::string& line) {
        std::vector<std::string> tokens;
        size_t i = 0;
        while (i < line.size()) {
            while (i < line.size() && line[i] == ' ')
                ++i;
            if (i >= line.size())
                break;
            size_t start = i;
            while (i < line.size() && line[i] != ' ')
                ++i;
            tokens.push_back(line.substr(start, i - start));
        }
        return tokens;
    }

    std::unordered_map<std::string, ReplCommand> commands_;
};
