#pragma once

#include "event/Event.h"

#include <string>

/// Published when the server sends the ASS (asset URL) packet.
/// The main thread should create an HTTP mount from this URL.
class AssetUrlEvent : public Event {
  public:
    AssetUrlEvent(std::string url) : url_(std::move(url)) {
    }

    std::string to_string() const override {
        return "AssetUrl: " + url_;
    }

    const std::string& url() const {
        return url_;
    }

  private:
    std::string url_;
};
