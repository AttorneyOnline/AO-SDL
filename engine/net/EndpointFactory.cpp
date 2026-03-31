#include "net/EndpointFactory.h"

#include "net/RestRouter.h"
#include "utils/Log.h"

void EndpointFactory::populate(RestRouter& router) {
    for (auto& [key, creator] : creators_) {
        auto ep = creator();
        Log::log_print(INFO, "REST: registered %s %s", ep->method().c_str(), ep->path_pattern().c_str());
        router.register_endpoint(std::move(ep));
    }
}
