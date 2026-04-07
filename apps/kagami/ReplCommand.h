#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct ServerContext;

/// Polymorphic base for REPL commands.
class ReplCommand {
  public:
    virtual ~ReplCommand() = default;
    virtual const std::string& name() const = 0;
    virtual const std::string& description() const = 0;
    virtual void execute(ServerContext& ctx, const std::vector<std::string>& args) = 0;
};

/// Convenience base that stores name/description, so derived classes
/// only need to implement execute().
class SimpleReplCommand : public ReplCommand {
  public:
    SimpleReplCommand(std::string name, std::string description)
        : name_(std::move(name)), description_(std::move(description)) {
    }

    const std::string& name() const override {
        return name_;
    }
    const std::string& description() const override {
        return description_;
    }

  private:
    std::string name_;
    std::string description_;
};

/// Registry of REPL commands. Dispatches input lines to handlers.
class ReplCommandRegistry {
  public:
    void add(std::unique_ptr<ReplCommand> cmd) {
        auto name = cmd->name();
        commands_[name] = std::move(cmd);
    }

    /// Try to dispatch a line. Returns false if the command was not found.
    bool dispatch(ServerContext& ctx, const std::string& line) const {
        auto tokens = tokenize(line);
        if (tokens.empty())
            return true;

        auto it = commands_.find(tokens[0]);
        if (it == commands_.end())
            return false;

        std::vector<std::string> args(tokens.begin() + 1, tokens.end());
        it->second->execute(ctx, args);
        return true;
    }

    const std::unordered_map<std::string, std::unique_ptr<ReplCommand>>& commands() const {
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

    std::unordered_map<std::string, std::unique_ptr<ReplCommand>> commands_;
};
