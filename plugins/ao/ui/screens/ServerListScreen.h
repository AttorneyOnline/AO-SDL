/**
 * @file ServerListScreen.h
 * @brief Server browser screen state.
 */
#pragma once

#include "game/ServerList.h"
#include "ui/Screen.h"

#include <vector>

/**
 * @brief Screen displaying the list of available servers for the user to browse and join.
 *
 * Consumes server list data and exposes read-only state for rendering backends.
 * When the user selects a server, select_server() records the selection and
 * handle_events() publishes a ServerConnectEvent to initiate the connection.
 */
class ServerListScreen : public Screen {
  public:
    /** @brief Unique screen identifier for backend dispatch. */
    static inline const std::string ID = "server_list";

    /**
     * @brief Called when this screen becomes active.
     *
     * Stores the controller reference and initializes server list data.
     *
     * @param controller Interface for screen stack navigation.
     */
    void enter(ScreenController& controller) override;

    /** @brief Called when this screen is deactivated or popped. */
    void exit() override;

    /**
     * @brief Process pending events for this frame.
     *
     * If a server connection is pending, publishes a ServerConnectEvent and
     * transitions to the next screen.
     */
    void handle_events() override;

    /**
     * @brief Get the screen identifier.
     * @return Reference to the static ID string "server_list".
     */
    const std::string& screen_id() const override {
        return ID;
    }

    /**
     * @brief Get the list of servers.
     * @return Const reference to the vector of ServerEntry items.
     */
    const std::vector<ServerEntry>& get_servers() const {
        return servers;
    }

    /**
     * @brief Get the index of the currently selected server.
     * @return Selected server index, or -1 if none is selected.
     */
    int get_selected() const {
        return selected;
    }

    /**
     * @brief Select a server and initiate connection.
     *
     * Records the selection and sets a pending connect flag. The actual
     * ServerConnectEvent is published on the next handle_events() call.
     *
     * @param index Zero-based index into the server list.
     */
    void select_server(int index);

    /// Direct connect to a host:port without going through the server list.
    void direct_connect(const std::string& host, uint16_t port);

  private:
    ScreenController* controller = nullptr; ///< Stored controller for stack navigation.
    std::vector<ServerEntry> servers;       ///< Cached list of available servers.
    int selected = -1;                      ///< Currently selected server index (-1 = none).
    bool pending_connect = false;           ///< True when a connection should be initiated next frame.
};
