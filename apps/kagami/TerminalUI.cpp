#include "TerminalUI.h"

#include <atomic>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#else
#include <csignal>
#include <sys/ioctl.h>
#include <unistd.h>
#endif

// ANSI color codes
static constexpr const char* RESET = "\033[0m";
static constexpr const char* DIM = "\033[2m";
static constexpr const char* BOLD = "\033[1m";
static constexpr const char* CYAN = "\033[36m";
static constexpr const char* GREEN = "\033[32m";
static constexpr const char* YELLOW = "\033[33m";
static constexpr const char* RED = "\033[31m";
static constexpr const char* MAGENTA = "\033[35m";
static constexpr const char* WHITE = "\033[37m";

static const char* level_color(LogLevel level) {
    switch (level) {
    case VERBOSE:
        return DIM;
    case DEBUG:
        return CYAN;
    case INFO:
        return GREEN;
    case WARNING:
        return YELLOW;
    case ERR:
        return RED;
    case FATAL:
        return MAGENTA;
    default:
        return WHITE;
    }
}

static void get_terminal_size(int& width, int& height) {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
        width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    }
#else
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
        width = ws.ws_col;
        height = ws.ws_row;
    }
#endif
}

// Global pointer for SIGWINCH handler (only one TerminalUI exists at a time).
static std::atomic<TerminalUI*> g_active_ui{nullptr};

#ifndef _WIN32
static void sigwinch_handler(int) {
    auto* ui = g_active_ui.load(std::memory_order_relaxed);
    if (ui)
        ui->handle_resize();
}
#endif

void TerminalUI::init() {
    get_terminal_size(width_, height_);
    std::cout << "\033[2J"; // clear screen
    apply_layout();

    g_active_ui.store(this, std::memory_order_relaxed);
#ifndef _WIN32
    struct sigaction sa = {};
    sa.sa_handler = sigwinch_handler;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGWINCH, &sa, nullptr);
#endif
}

void TerminalUI::cleanup() {
#ifndef _WIN32
    signal(SIGWINCH, SIG_DFL);
#endif
    g_active_ui.store(nullptr, std::memory_order_relaxed);

    std::cout << "\033[r";                      // reset scroll region
    std::cout << "\033[" << height_ << ";1H\n"; // move below UI
    std::cout << std::flush;
}

void TerminalUI::handle_resize() {
    std::lock_guard lock(mutex_);
    get_terminal_size(width_, height_);
    apply_layout();
}

void TerminalUI::apply_layout() {
    int scroll_bottom = height_ - 2;
    std::cout << "\033[1;" << scroll_bottom << "r"; // set scroll region
    std::cout << "\033[" << scroll_bottom << ";1H"; // park cursor
    draw_separator();
    show_prompt();
}

void TerminalUI::log(LogLevel level, const std::string& timestamp, const std::string& message) {
    const char* color = level_color(level);
    std::string line;
    line += DIM;
    line += timestamp;
    line += " ";
    line += color;
    line += BOLD;
    line += log_level_name(level);
    line += RESET;
    line += " ";
    line += message;
    emit(line);
}

void TerminalUI::print(const std::string& line) {
    std::string formatted;
    formatted += CYAN;
    formatted += "  \u25b8 "; // ▸
    formatted += RESET;
    formatted += line;
    emit(formatted);
}

void TerminalUI::show_prompt() {
    std::cout << "\033[" << height_ << ";1H";
    std::cout << "\033[2K";
    std::cout << BOLD << "> " << RESET << std::flush;
}

void TerminalUI::emit(const std::string& line) {
    std::lock_guard lock(mutex_);

    int scroll_bottom = height_ - 2;

    std::cout << "\033[" << scroll_bottom << ";1H";
    std::cout << "\n";
    std::cout << "\033[" << scroll_bottom << ";1H";
    std::cout << "\033[2K";
    std::cout << line << RESET;

    draw_separator();
    show_prompt();
    std::cout << std::flush;
}

void TerminalUI::draw_separator() {
    std::cout << "\033[" << (height_ - 1) << ";1H";
    std::cout << "\033[2K";
    std::cout << DIM;
    for (int i = 0; i < width_; ++i)
        std::cout << "\u2500";
    std::cout << RESET << std::flush;
}
