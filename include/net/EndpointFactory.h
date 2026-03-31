#pragma once

#include "net/RestEndpoint.h"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

class RestRouter;

/// Singleton registry of REST endpoint creators.
/// Parallels PacketFactory: plugins register creators at static-init time,
/// then populate() instantiates all endpoints and transfers them to the router.
class EndpointFactory {
  public:
    using CreatorFunc = std::function<std::unique_ptr<RestEndpoint>()>;

    static EndpointFactory& instance() {
        static EndpointFactory factory;
        return factory;
    }

    void register_endpoint(const std::string& key, CreatorFunc creator) {
        creators_[key] = std::move(creator);
    }

    /// Instantiate all registered endpoints and add them to the router.
    void populate(RestRouter& router);

  private:
    std::unordered_map<std::string, CreatorFunc> creators_;
};
