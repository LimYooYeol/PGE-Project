#include "pgenet_packetmanager.h"

#include "packetV2/packets/ClientToServer/packetuserauth.h"

#include "packetV2/packets/Both/packetmessage.h"

PGENET_PacketManager::PGENET_PacketManager()
{
    registerAllPackets();
}


PGENET_DefPacketRegister *PGENET_PacketManager::getPacketRegister()
{
    return &m_packetRegister;
}

void PGENET_PacketManager::registerAllPackets()
{
    m_packetRegister.registerPacket<PacketUserAuth>(PacketID::PGENET_PacketUserAuth);
    m_packetRegister.registerPacket<PacketMessage>(PacketID::PGENET_PacketMessage);
}


