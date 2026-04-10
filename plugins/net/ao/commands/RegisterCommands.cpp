/// Linker anchors for OOC command registrars.
/// Each command .cpp file exports a void ao_cmd_<name>() function.
/// Calling them here forces the linker to include those TUs,
/// which in turn runs their static CommandRegistrar constructors.

void ao_cmd_adduser();
void ao_cmd_area();
void ao_cmd_arealock();
void ao_cmd_asn();
void ao_cmd_background();
void ao_cmd_ban();
void ao_cmd_editban();
void ao_cmd_searchbans();
void ao_cmd_changeauth();
void ao_cmd_changepass();
void ao_cmd_cm();
void ao_cmd_firewall();
void ao_cmd_getarea();
void ao_cmd_help();
void ao_cmd_kick();
void ao_cmd_listperms();
void ao_cmd_listusers();
void ao_cmd_login();
void ao_cmd_logout();
void ao_cmd_motd();
void ao_cmd_online();
void ao_cmd_removeperms();
void ao_cmd_removeuser();
void ao_cmd_reputation();
void ao_cmd_rootpass();
void ao_cmd_setperms();
void ao_cmd_status();
void ao_cmd_testimony();
void ao_cmd_unban();
void ao_cmd_modheat();

void ao_register_commands() {
    ao_cmd_adduser();
    ao_cmd_area();
    ao_cmd_arealock();
    ao_cmd_asn();
    ao_cmd_background();
    ao_cmd_ban();
    ao_cmd_editban();
    ao_cmd_searchbans();
    ao_cmd_changeauth();
    ao_cmd_changepass();
    ao_cmd_cm();
    ao_cmd_firewall();
    ao_cmd_getarea();
    ao_cmd_help();
    ao_cmd_kick();
    ao_cmd_listperms();
    ao_cmd_listusers();
    ao_cmd_login();
    ao_cmd_logout();
    ao_cmd_motd();
    ao_cmd_online();
    ao_cmd_removeperms();
    ao_cmd_removeuser();
    ao_cmd_reputation();
    ao_cmd_rootpass();
    ao_cmd_setperms();
    ao_cmd_status();
    ao_cmd_testimony();
    ao_cmd_unban();
    ao_cmd_modheat();
}
