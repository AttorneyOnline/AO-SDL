#pragma once

#include "AOPacket.h"
#include "PacketRegistrar.h"

class AOClient;

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

  private:
    std::string hardware_id;

    static PacketRegistrar registrar;
    static constexpr int MIN_FIELDS = 1;
};

class AOPacketIDClient : public AOPacket {
  public:
    AOPacketIDClient(const std::vector<std::string>& fields);

    virtual void handle(AOClient& cli) override;

  private:
    int player_number;
    std::string server_software;
    std::string server_version;

    static PacketRegistrar registrar;
    static constexpr int MIN_FIELDS = 3;
};

class AOPacketIDServer : public AOPacket {
  public:
    AOPacketIDServer(const std::string& client_software, const std::string& client_version);
    AOPacketIDServer(const std::vector<std::string>& fields);

  private:
    std::string client_software;
    std::string client_version;

    static constexpr int MIN_FIELDS = 2;
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

  private:
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

  private:
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

  private:
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

  private:
    static constexpr int MIN_FIELDS = 0;
};

/// Keepalive heartbeat (client → server). Sent every ~45s.
class AOPacketCH : public AOPacket {
  public:
    AOPacketCH(int char_id);
  private:
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

  private:
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

class AOPacketCT : public AOPacket {
  public:
    AOPacketCT(const std::string& sender_name, const std::string& message, bool system_message);
    AOPacketCT(const std::vector<std::string>& fields);

    virtual void handle(AOClient& cli) override;

  private:
    std::string sender_name;
    std::string message;
    bool system_message;

    static PacketRegistrar registrar;
    static constexpr int MIN_FIELDS = 2;
};
