#ifndef PACKETTYPES_H
#define PACKETTYPES_H

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

class AOPacketDONE : public AOPacket {
  public:
    AOPacketDONE();

    virtual void handle(AOClient& cli) override;

  private:
    static PacketRegistrar registrar;
    static constexpr int MIN_FIELDS = 0;
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

#endif
