/// Linker anchor: ensures all CommandRegistrar statics in this directory
/// are linked. Same pattern as ao_register_packet_types().
void ao_register_commands() {
    // The static CommandRegistrar objects in the other .cpp files
    // register all commands at program startup. This function exists
    // solely to create a linker dependency so the TUs aren't stripped.
}
