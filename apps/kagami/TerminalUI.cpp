#include "TerminalUI.h"

#include <iostream>

#ifdef _WIN32
#include <windows.h>
#else
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

void TerminalUI::init() {
    get_terminal_size(width_, height_);

    // Clear screen
    std::cout << "\033[2J";

    // Set scroll region to rows 1 through H-2 (leave 2 rows for separator + prompt)
    int scroll_bottom = height_ - 2;
    std::cout << "\033[1;" << scroll_bottom << "r";

    // Park cursor at the bottom of the scroll region so the first
    // log line triggers a scroll instead of landing on the separator.
    std::cout << "\033[" << scroll_bottom << ";1H";

    draw_separator();
    show_prompt();
}

void TerminalUI::cleanup() {
    // Reset scroll region to full screen
    std::cout << "\033[r";
    // Move cursor below the UI
    std::cout << "\033[" << height_ << ";1H\n";
    std::cout << std::flush;
}

void TerminalUI::log(LogLevel level, const std::string& timestamp, const std::string& message) {
    const char* color = level_color(level);
    std::string line;
    // Dim timestamp, colored level tag, white message
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
    // REPL output: cyan arrow prefix to distinguish from log lines
    std::string formatted;
    formatted += CYAN;
    formatted += "  \u25b8 "; // ▸
    formatted += RESET;
    formatted += line;
    emit(formatted);
}

void TerminalUI::show_prompt() {
    std::cout << "\033[" << height_ << ";1H"; // move to last row
    std::cout << "\033[2K";                   // clear line
    std::cout << BOLD << "> " << RESET << std::flush;
}

void TerminalUI::emit(const std::string& line) {
    std::lock_guard lock(mutex_);

    int scroll_bottom = height_ - 2;

    // Move into the scroll region and force a scroll-up by writing at the bottom.
    // \033[T scrolls the region down (inserts blank at top), but we want scroll up.
    // The reliable way: move to the last line, issue \n to scroll, then overwrite.
    std::cout << "\033[" << scroll_bottom << ";1H"; // move to last row of scroll region
    std::cout << "\n";                              // scroll region up by one line
    std::cout << "\033[" << scroll_bottom << ";1H"; // move back (now a blank line)
    std::cout << "\033[2K";                         // clear it
    std::cout << line << RESET;                     // print content

    // Redraw separator and prompt since cursor moved out of prompt area
    draw_separator();
    show_prompt();
    std::cout << std::flush;
}

void TerminalUI::draw_separator() {
    std::cout << "\033[" << (height_ - 1) << ";1H";
    std::cout << "\033[2K";
    std::cout << DIM;
    for (int i = 0; i < width_; ++i)
        std::cout << "\u2500"; // ─
    std::cout << RESET << std::flush;
}
