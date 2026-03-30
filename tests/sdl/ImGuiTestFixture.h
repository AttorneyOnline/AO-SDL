#pragma once

#include "NullGPUBackend.h"
#include "StubRenderer.h"

#include "event/EventManager.h"
#include "ui/widgets/CourtroomState.h"

#include <gtest/gtest.h>
#include <imgui.h>

/// RAII test fixture that creates an ImGui context with the null backend.
/// Provides begin_frame()/end_frame() helpers so widget render() can be
/// called inside a valid ImGui frame.
class ImGuiTestFixture : public ::testing::Test {
  protected:
    void SetUp() override {
        ctx_ = ImGui::CreateContext();
        backend_.init_imgui(nullptr, renderer_);
        CourtroomState::instance().reset();
    }

    void TearDown() override {
        CourtroomState::instance().reset();
        backend_.shutdown();
        ImGui::DestroyContext(ctx_);
    }

    void begin_frame() {
        backend_.begin_frame();
    }

    void end_frame() {
        backend_.present();
    }

    /// Run a callable inside a valid ImGui frame.
    template <typename F>
    void with_frame(F&& fn) {
        begin_frame();
        fn();
        end_frame();
    }

    /// Drain all pending events from a channel.
    template <typename T>
    void drain() {
        auto& ch = EventManager::instance().get_channel<T>();
        while (ch.get_event()) {
        }
    }

  private:
    ImGuiContext* ctx_ = nullptr;
    NullGPUBackend backend_;
    StubRenderer renderer_;
};
