/**
 * @file Session.h
 * @brief Scoped lifetime for a single server connection.
 *
 * A Session owns all resources that should be cleaned up when disconnecting
 * from a server: HTTP mounts, session-tagged cache entries, and audio state.
 * Destroying the Session automatically tears down all of these.
 */
#pragma once

#include "asset/MountManager.h"

#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

class AssetLibrary;
class HttpPool;
class Mount;

class Session {
  public:
    Session(MountManager& mounts, AssetLibrary& assets);
    ~Session();

    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;

    /// Add an HTTP mount for this session. The mount is removed on destruction.
    MountManager::MountHandle add_http_mount(const std::string& url, HttpPool& pool);

    uint32_t session_id() const {
        return session_id_;
    }

  private:
    MountManager& mounts_;
    AssetLibrary& assets_;
    uint32_t session_id_;
    std::vector<MountManager::MountHandle> mount_handles_;

    static std::atomic<uint32_t> next_session_id_;
};
