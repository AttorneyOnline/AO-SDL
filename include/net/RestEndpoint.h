#pragma once

#include "net/RestRequest.h"
#include "net/RestResponse.h"

#include <string>

/// Abstract base for REST endpoint handlers.
/// Concrete endpoints override this to declare their route and implement
/// request handling. Parallels AOPacket in the AO2 protocol layer.
class RestEndpoint {
  public:
    virtual ~RestEndpoint() = default;

    virtual const std::string& method() const = 0;
    virtual const std::string& path_pattern() const = 0;
    virtual bool requires_auth() const = 0;

    virtual RestResponse handle(const RestRequest& req) = 0;
};
