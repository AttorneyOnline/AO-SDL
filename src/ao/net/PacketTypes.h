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

#endif