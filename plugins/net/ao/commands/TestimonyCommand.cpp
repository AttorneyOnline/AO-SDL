#include "game/AreaState.h"
#include "game/CommandContext.h"
#include "game/CommandHandler.h"
#include "game/CommandRegistrar.h"
#include "game/GameRoom.h"
#include "game/ServerSession.h"

#include "utils/Log.h"

/// Helper: get area state or send error.
static AreaState* get_area_or_fail(CommandContext& ctx) {
    auto* area = ctx.room.find_area_by_name(ctx.session.area);
    if (!area)
        ctx.send_system_message("Area not found.");
    return area;
}

/// Helper: check CM/mod permission.
static bool require_cm(CommandContext& ctx, AreaState* area) {
    if (area->is_cm(ctx.session.client_id) || ctx.session.moderator)
        return true;
    ctx.send_system_message("You must be CM or moderator.");
    return false;
}

/// /testify — start recording testimony.
class TestifyCommand : public CommandHandler {
  public:
    const std::string& name() const override {
        static const std::string n = "testify";
        return n;
    }

    std::string usage() const override {
        return "/testify";
    }

    void execute(CommandContext& ctx) override {
        auto* area = get_area_or_fail(ctx);
        if (!area || !require_cm(ctx, area))
            return;

        area->testimony.reset();
        area->testimony.state = TestimonyState::RECORDING;
        ctx.send_system_message("Testimony recording started. Each IC message will be recorded as a statement.");
        Log::log_print(INFO, "Testimony: recording started in %s by %s", ctx.session.area.c_str(),
                       ctx.session.display_name.c_str());
    }
};

/// /testimony — display the current testimony statements.
class TestimonyDisplayCommand : public CommandHandler {
  public:
    const std::string& name() const override {
        static const std::string n = "testimony";
        return n;
    }

    std::string usage() const override {
        return "/testimony";
    }

    void execute(CommandContext& ctx) override {
        auto* area = get_area_or_fail(ctx);
        if (!area)
            return;

        auto& t = area->testimony;
        if (t.statements.empty()) {
            ctx.send_system_message("No testimony recorded.");
            return;
        }

        const char* state_str = "IDLE";
        if (t.state == TestimonyState::RECORDING)
            state_str = "RECORDING";
        else if (t.state == TestimonyState::PLAYBACK)
            state_str = "PLAYBACK";
        else if (t.state == TestimonyState::UPDATE)
            state_str = "UPDATE";
        else if (t.state == TestimonyState::ADD)
            state_str = "ADD";

        std::string text =
            "Testimony (" + std::string(state_str) + "), " + std::to_string(t.statements.size()) + " statement(s):";
        for (int i = 0; i < static_cast<int>(t.statements.size()); ++i) {
            text += "\n  " + std::to_string(i + 1) + ". ";
            if (i == t.current_index)
                text += ">> ";
            // Show a brief preview (first 60 chars of the raw data)
            auto& s = t.statements[i];
            text += s.size() > 60 ? s.substr(0, 60) + "..." : s;
        }
        ctx.send_system_message(text);
    }
};

/// /examine — start cross-examination (testimony playback).
class ExamineCommand : public CommandHandler {
  public:
    const std::string& name() const override {
        static const std::string n = "examine";
        return n;
    }

    std::string usage() const override {
        return "/examine";
    }

    void execute(CommandContext& ctx) override {
        auto* area = get_area_or_fail(ctx);
        if (!area || !require_cm(ctx, area))
            return;

        if (area->testimony.statements.empty()) {
            ctx.send_system_message("No testimony to examine.");
            return;
        }

        area->testimony.state = TestimonyState::PLAYBACK;
        area->testimony.current_index = 0;
        ctx.send_system_message("Cross-examination started. Use > and < in IC to navigate statements.");
    }
};

/// /pause — stop testimony recording/playback.
class PauseCommand : public CommandHandler {
  public:
    const std::string& name() const override {
        static const std::string n = "pause";
        return n;
    }

    std::string usage() const override {
        return "/pause";
    }

    void execute(CommandContext& ctx) override {
        auto* area = get_area_or_fail(ctx);
        if (!area || !require_cm(ctx, area))
            return;

        area->testimony.state = TestimonyState::IDLE;
        ctx.send_system_message("Testimony paused.");
    }
};

/// /delete — delete the current statement from testimony.
class DeleteStatementCommand : public CommandHandler {
  public:
    const std::string& name() const override {
        static const std::string n = "delete";
        return n;
    }

    std::string usage() const override {
        return "/delete";
    }

    void execute(CommandContext& ctx) override {
        auto* area = get_area_or_fail(ctx);
        if (!area || !require_cm(ctx, area))
            return;

        auto& t = area->testimony;
        if (t.current_index < 0 || t.current_index >= static_cast<int>(t.statements.size())) {
            ctx.send_system_message("No statement selected to delete.");
            return;
        }

        t.statements.erase(t.statements.begin() + t.current_index);
        if (t.current_index >= static_cast<int>(t.statements.size()))
            t.current_index = static_cast<int>(t.statements.size()) - 1;

        ctx.send_system_message("Statement deleted. " + std::to_string(t.statements.size()) + " remaining.");
    }
};

/// /update — replace the current statement (next IC message becomes the new one).
class UpdateStatementCommand : public CommandHandler {
  public:
    const std::string& name() const override {
        static const std::string n = "update";
        return n;
    }

    std::string usage() const override {
        return "/update";
    }

    void execute(CommandContext& ctx) override {
        auto* area = get_area_or_fail(ctx);
        if (!area || !require_cm(ctx, area))
            return;

        auto& t = area->testimony;
        if (t.current_index < 0 || t.current_index >= static_cast<int>(t.statements.size())) {
            ctx.send_system_message("No statement selected to update.");
            return;
        }

        t.state = TestimonyState::UPDATE;
        ctx.send_system_message("Send your next IC message to replace statement " +
                                std::to_string(t.current_index + 1) + ".");
    }
};

/// /add — add a new statement after the current position.
class AddStatementCommand : public CommandHandler {
  public:
    const std::string& name() const override {
        static const std::string n = "add";
        return n;
    }

    std::string usage() const override {
        return "/add";
    }

    void execute(CommandContext& ctx) override {
        auto* area = get_area_or_fail(ctx);
        if (!area || !require_cm(ctx, area))
            return;

        area->testimony.state = TestimonyState::ADD;
        ctx.send_system_message("Send your next IC message to add a new statement.");
    }
};

static CommandRegistrar reg1(std::make_unique<TestifyCommand>());
static CommandRegistrar reg2(std::make_unique<TestimonyDisplayCommand>());
static CommandRegistrar reg3(std::make_unique<ExamineCommand>());
static CommandRegistrar reg4(std::make_unique<PauseCommand>());
static CommandRegistrar reg5(std::make_unique<DeleteStatementCommand>());
static CommandRegistrar reg6(std::make_unique<UpdateStatementCommand>());
static CommandRegistrar reg7(std::make_unique<AddStatementCommand>());
