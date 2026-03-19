#include "ao_plugin.h"

#include "ao/net/AOClient.h"

namespace ao {

std::unique_ptr<ProtocolHandler> create_protocol() {
    return std::make_unique<AOClient>();
}

} // namespace ao
