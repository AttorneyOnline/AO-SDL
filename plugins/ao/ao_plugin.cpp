#include "ao_plugin.h"

#include "ao/game/AOCourtroomPresenter.h"
#include "net/ao/AOClient.h"

namespace ao {

std::unique_ptr<ProtocolHandler> create_protocol() {
    return std::make_unique<AOClient>();
}

std::unique_ptr<IScenePresenter> create_presenter() {
    return std::make_unique<AOCourtroomPresenter>();
}

} // namespace ao
