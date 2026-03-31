#include "NXEndpoint.h"

// Linker anchors defined in each endpoint TU.
void nx_ep_server_info();
void nx_ep_server_motd();
void nx_ep_server_players();
void nx_ep_session_create();
void nx_ep_session_delete();
void nx_ep_session_renew();

// Phase 3: Character & Area endpoints (#90)
void nx_ep_character_list();
void nx_ep_character_get();
void nx_ep_character_select();
void nx_ep_area_list();
void nx_ep_area_get();
void nx_ep_area_players();

// Force all endpoint TUs to link. Same pattern as ao_register_packet_types().
void nx_register_endpoints() {
    nx_ep_server_info();
    nx_ep_server_motd();
    nx_ep_server_players();
    nx_ep_session_create();
    nx_ep_session_delete();
    nx_ep_session_renew();

    nx_ep_character_list();
    nx_ep_character_get();
    nx_ep_character_select();
    nx_ep_area_list();
    nx_ep_area_get();
    nx_ep_area_players();
}
