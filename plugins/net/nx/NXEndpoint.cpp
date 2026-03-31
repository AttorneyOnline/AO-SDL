#include "NXEndpoint.h"

// Linker anchors defined in each endpoint TU.
void nx_ep_server_motd();
void nx_ep_server_players();
void nx_ep_session_create();
void nx_ep_session_delete();
void nx_ep_session_renew();

// Force all endpoint TUs to link. Same pattern as ao_register_packet_types().
void nx_register_endpoints() {
    nx_ep_server_motd();
    nx_ep_server_players();
    nx_ep_session_create();
    nx_ep_session_delete();
    nx_ep_session_renew();
}
