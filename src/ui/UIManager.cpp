#include "UIManager.h"

#include "ui/screens/ServerListScreen.h"

UIManager::UIManager() {
    push_screen(std::make_unique<ServerListScreen>());
}

void UIManager::push_screen(std::unique_ptr<Screen> screen) {
    if (!m_stack.empty()) {
        m_stack.back()->exit();
    }
    m_stack.push_back(std::move(screen));
    m_stack.back()->enter(*this);
}

void UIManager::pop_screen() {
    if (m_stack.empty()) return;
    m_stack.back()->exit();
    m_stack.pop_back();
    if (!m_stack.empty()) {
        m_stack.back()->enter(*this);
    }
}

Screen* UIManager::top() const {
    return m_stack.empty() ? nullptr : m_stack.back().get();
}

void UIManager::handle_events() {
    if (auto* s = top()) s->handle_events();
}

void UIManager::render(RenderManager& render) {
    if (auto* s = top()) s->render(render);
}
