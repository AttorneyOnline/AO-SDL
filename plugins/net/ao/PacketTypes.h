#pragma once

#include "AOPacket.h"
#include "PacketRegistrar.h"

class AOClient;

/// Call once at startup to ensure all PacketRegistrar statics are initialized.
/// This prevents the linker from stripping the translation unit when linking
/// as a static library.
void ao_register_packet_types();

// To prevent footguns, only implement the constructor to specify explicit fields if the client is supposed to send it

class AOPacketDecryptor : public AOPacket {
  public:
    AOPacketDecryptor(const std::vector<std::string>& fields);

    virtual void handle(AOClient& cli) override;

  private:
    std::string decryptor;

    static PacketRegistrar registrar;
    static constexpr int MIN_FIELDS = 1;
};

class AOPacketHI : public AOPacket {
  public:
    AOPacketHI(const std::string& hardware_id);
    AOPacketHI(const std::vector<std::string>& fields);
    void handle_server(AOServer& server, ServerSession& session) override;

  private:
    std::string hardware_id;

    static PacketRegistrar registrar;
    static constexpr int MIN_FIELDS = 1;
};

class AOPacketIDClient : public AOPacket {
  public:
    AOPacketIDClient(const std::vector<std::string>& fields);

    virtual void handle(AOClient& cli) override;
    void handle_server(AOServer& server, ServerSession& session) override;

  private:
    int player_number = 0;
    std::string server_software;
    std::string server_version;

    static PacketRegistrar registrar;
    static constexpr int MIN_FIELDS = 2; // 2 for client→server, 3 for server→client
};

class AOPacketIDServer : public AOPacket {
  public:
    AOPacketIDServer(const std::string& client_software, const std::string& client_version);

  private:
    std::string client_software;
    std::string client_version;
};

class AOPacketPN : public AOPacket {
  public:
    AOPacketPN(const std::vector<std::string>& fields);

    virtual void handle(AOClient& cli) override;

  private:
    int current_players;
    int max_players;
    std::string server_description;

    static PacketRegistrar registrar;
    static constexpr int MIN_FIELDS = 2;
};

class AOPacketAskChaa : public AOPacket {
  public:
    AOPacketAskChaa();
    AOPacketAskChaa(const std::vector<std::string>& fields);
    void handle_server(AOServer& server, ServerSession& session) override;

  private:
    static PacketRegistrar registrar;
    static constexpr int MIN_FIELDS = 0;
};

class AOPacketASS : public AOPacket {
  public:
    AOPacketASS(const std::vector<std::string>& fields);

    virtual void handle(AOClient& cli) override;

  private:
    std::string asset_url;

    static PacketRegistrar registrar;
    static constexpr int MIN_FIELDS = 1;
};

class AOPacketSI : public AOPacket {
  public:
    AOPacketSI(const std::vector<std::string>& fields);

    virtual void handle(AOClient& cli) override;

  private:
    int character_count;
    int evidence_count;
    int music_count;

    static PacketRegistrar registrar;
    static constexpr int MIN_FIELDS = 3;
};

class AOPacketRC : public AOPacket {
  public:
    AOPacketRC();
    AOPacketRC(const std::vector<std::string>& fields);
    void handle_server(AOServer& server, ServerSession& session) override;

  private:
    static PacketRegistrar registrar;
    static constexpr int MIN_FIELDS = 0;
};

class AOPacketSC : public AOPacket {
  public:
    AOPacketSC(const std::vector<std::string>& fields);

    virtual void handle(AOClient& cli) override;

  private:
    std::vector<std::string> character_list;

    static PacketRegistrar registrar;
    static constexpr int MIN_FIELDS = 1;
};

class AOPacketRM : public AOPacket {
  public:
    AOPacketRM();
    AOPacketRM(const std::vector<std::string>& fields);
    void handle_server(AOServer& server, ServerSession& session) override;

  private:
    static PacketRegistrar registrar;
    static constexpr int MIN_FIELDS = 0;
};

class AOPacketSM : public AOPacket {
  public:
    AOPacketSM(const std::vector<std::string>& fields);

    virtual void handle(AOClient& cli) override;

  private:
    std::vector<std::string> music_list;

    static PacketRegistrar registrar;
    static constexpr int MIN_FIELDS = 1;
};

class AOPacketRD : public AOPacket {
  public:
    AOPacketRD();
    AOPacketRD(const std::vector<std::string>& fields);
    void handle_server(AOServer& server, ServerSession& session) override;

  private:
    static PacketRegistrar registrar;
    static constexpr int MIN_FIELDS = 0;
};

/// Keepalive heartbeat (client → server). Sent every ~45s.
class AOPacketCH : public AOPacket {
  public:
    AOPacketCH(int char_id);
    AOPacketCH(const std::vector<std::string>& fields);
    void handle_server(AOServer& server, ServerSession& session) override;

  private:
    static PacketRegistrar registrar;
    static constexpr int MIN_FIELDS = 1;
};

/// Keepalive response (server → client). Received in response to CH.
class AOPacketCHECK : public AOPacket {
  public:
    AOPacketCHECK(const std::vector<std::string>& fields);
    virtual void handle(AOClient& cli) override;

  private:
    static PacketRegistrar registrar;
    static constexpr int MIN_FIELDS = 0;
};

class AOPacketDONE : public AOPacket {
  public:
    AOPacketDONE();

    virtual void handle(AOClient& cli) override;

  private:
    static PacketRegistrar registrar;
    static constexpr int MIN_FIELDS = 0;
};

class AOPacketCharsCheck : public AOPacket {
  public:
    AOPacketCharsCheck(const std::vector<std::string>& fields);

    virtual void handle(AOClient& cli) override;

  private:
    std::vector<bool> taken;

    static PacketRegistrar registrar;
    static constexpr int MIN_FIELDS = 1;
};

class AOPacketPW : public AOPacket {
  public:
    AOPacketPW(const std::string& password);

  private:
    static constexpr int MIN_FIELDS = 1;
};

class AOPacketCC : public AOPacket {
  public:
    AOPacketCC(int player_num, int char_id, const std::string& hdid);
    AOPacketCC(const std::vector<std::string>& fields);
    void handle_server(AOServer& server, ServerSession& session) override;

  private:
    static PacketRegistrar registrar;
    static constexpr int MIN_FIELDS = 3;
};

class AOPacketPV : public AOPacket {
  public:
    AOPacketPV(const std::vector<std::string>& fields);

    virtual void handle(AOClient& cli) override;

  private:
    int char_id = -1;

    static PacketRegistrar registrar;
    static constexpr int MIN_FIELDS = 3;
};

struct ICMessageData;

class AOPacketMS : public AOPacket {
  public:
    explicit AOPacketMS(const ICMessageData& data);
    AOPacketMS(const std::vector<std::string>& fields);

    virtual void handle(AOClient& cli) override;
    void handle_server(AOServer& server, ServerSession& session) override;

  private:
    int desk_mod;
    std::string pre_emote;
    std::string character;
    std::string emote;
    std::string message;
    std::string side;
    std::string showname;
    int emote_mod;
    int char_id;
    bool flip;
    int text_color;
    int objection_mod = 0;
    bool screenshake = false;
    bool realization = false;
    bool additive = false;
    std::string frame_screenshake;
    std::string sfx_name;
    int sfx_delay = 0;
    bool sfx_looping = false;
    std::string frame_sfx;
    bool immediate = false;
    bool slide = false;

    static PacketRegistrar registrar;
    static constexpr int MIN_FIELDS = 15;
};

class AOPacketBN : public AOPacket {
  public:
    AOPacketBN(const std::vector<std::string>& fields);

    virtual void handle(AOClient& cli) override;

  private:
    std::string background;
    std::string position;

    static PacketRegistrar registrar;
    static constexpr int MIN_FIELDS = 1;
};

/// Music/area change. Sent by client to request a change, received from server
/// to broadcast that someone changed the music or switched areas.
class AOPacketMC : public AOPacket {
  public:
    /// Outgoing: client requests music/area change.
    AOPacketMC(const std::string& name, int char_id, const std::string& showname = "");
    /// Incoming: server broadcasts music/area change.
    AOPacketMC(const std::vector<std::string>& fields);

    virtual void handle(AOClient& cli) override;

  private:
    std::string name;
    int char_id = -1;
    std::string showname;
    int looping = -1;     // -1=unknown, 0=no, 1=yes
    int channel = 0;      // 0=music, 1+=ambience
    int effect_flags = 0; // bitmask: bit0=fade_in, bit1=fade_out, bit2=sync

    static PacketRegistrar registrar;
    static constexpr int MIN_FIELDS = 2;
};

/// Area update packet. Sent by server to update one metadata field for all areas.
class AOPacketARUP : public AOPacket {
  public:
    AOPacketARUP(const std::vector<std::string>& fields);
    virtual void handle(AOClient& cli) override;

  private:
    int arup_type = 0;
    std::vector<std::string> values;

    static PacketRegistrar registrar;
    static constexpr int MIN_FIELDS = 2;
};

/// Feature list (server → client). Lists supported protocol features.
class AOPacketFL : public AOPacket {
  public:
    AOPacketFL(const std::vector<std::string>& fields);
    virtual void handle(AOClient& cli) override;

  private:
    std::vector<std::string> features;
    static PacketRegistrar registrar;
    static constexpr int MIN_FIELDS = 0;
};

/// Fetch areas (server → client). Replaces area list dynamically.
class AOPacketFA : public AOPacket {
  public:
    AOPacketFA(const std::vector<std::string>& fields);
    virtual void handle(AOClient& cli) override;

  private:
    std::vector<std::string> areas;
    static PacketRegistrar registrar;
    static constexpr int MIN_FIELDS = 0;
};

/// Fetch music (server → client). Replaces music list dynamically.
class AOPacketFM : public AOPacket {
  public:
    AOPacketFM(const std::vector<std::string>& fields);
    virtual void handle(AOClient& cli) override;

  private:
    std::vector<std::string> tracks;
    static PacketRegistrar registrar;
    static constexpr int MIN_FIELDS = 0;
};

/// Health/penalty bar (server → client). side: 1=def, 2=pro. value: 0-10.
class AOPacketHP : public AOPacket {
  public:
    AOPacketHP(const std::vector<std::string>& fields);
    virtual void handle(AOClient& cli) override;

  private:
    int side = 0;
    int value = 0;
    static PacketRegistrar registrar;
    static constexpr int MIN_FIELDS = 2;
};

/// Timer control (server → client).
/// action: 0=start/sync, 1=pause, 2=show, 3=hide.
class AOPacketTI : public AOPacket {
  public:
    AOPacketTI(const std::vector<std::string>& fields);
    virtual void handle(AOClient& cli) override;

  private:
    int timer_id = 0;
    int action = 0;
    int64_t time_ms = 0;
    static PacketRegistrar registrar;
    static constexpr int MIN_FIELDS = 2;
};

/// List evidence (server → client). Each field is "name&description&image".
class AOPacketLE : public AOPacket {
  public:
    AOPacketLE(const std::vector<std::string>& fields);
    virtual void handle(AOClient& cli) override;

  private:
    std::vector<std::string> raw_items;
    static PacketRegistrar registrar;
    static constexpr int MIN_FIELDS = 0;
};

/// Player register (server → client). type: 0=add, 1=remove.
class AOPacketPR : public AOPacket {
  public:
    AOPacketPR(const std::vector<std::string>& fields);
    virtual void handle(AOClient& cli) override;

  private:
    int player_id = 0;
    int update_type = 0;
    static PacketRegistrar registrar;
    static constexpr int MIN_FIELDS = 2;
};

/// Player update (server → client). data_type: 0=name, 1=character, 2=charname, 3=area.
class AOPacketPU : public AOPacket {
  public:
    AOPacketPU(const std::vector<std::string>& fields);
    virtual void handle(AOClient& cli) override;

  private:
    int player_id = 0;
    int data_type = 0;
    std::string data;
    static PacketRegistrar registrar;
    static constexpr int MIN_FIELDS = 3;
};

class AOPacketCT : public AOPacket {
  public:
    AOPacketCT(const std::string& sender_name, const std::string& message, bool system_message);
    AOPacketCT(const std::vector<std::string>& fields);

    virtual void handle(AOClient& cli) override;
    void handle_server(AOServer& server, ServerSession& session) override;

  private:
    std::string sender_name;
    std::string message;
    bool system_message;

    static PacketRegistrar registrar;
    static constexpr int MIN_FIELDS = 2;
};
