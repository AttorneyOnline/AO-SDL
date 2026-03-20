#include "ui/UIManager.h"

UIManager::UIManager() {
}

void UIManager::push_screen(std::unique_ptr<Screen> screen) {
    if (!stack.empty()) {
        stack.back()->exit();
    }
    stack.push_back(std::move(screen));
    stack.back()->enter(*this);
}

void UIManager::pop_screen() {
    if (stack.empty())
        return;
    stack.back()->exit();
    stack.pop_back();
    if (!stack.empty()) {
        stack.back()->enter(*this);
    }
}

Screen* UIManager::active_screen() const {
    return stack.empty() ? nullptr : stack.back().get();
}

void UIManager::pop_to_root() {
    while (stack.size() > 1) {
        stack.back()->exit();
        stack.pop_back();
    }
    if (!stack.empty()) {
        stack.back()->enter(*this);
    }
}

void UIManager::handle_events() {
    if (auto* s = active_screen())
        s->handle_events();
}
