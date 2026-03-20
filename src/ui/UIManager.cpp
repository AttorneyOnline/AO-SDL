#include "UIManager.h"

#include "ui/screens/ServerListScreen.h"

UIManager::UIManager() {
    push_screen(std::make_unique<ServerListScreen>());
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

Screen* UIManager::top() const {
    return stack.empty() ? nullptr : stack.back().get();
}

void UIManager::handle_events() {
    if (auto* s = top())
        s->handle_events();
}

void UIManager::render(RenderManager& render) {
    if (auto* s = top())
        s->render(render);
}
