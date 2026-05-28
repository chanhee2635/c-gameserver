#pragma once
#include "Session.h"
#include "GatePacket.pb.h"

template<auto PacketType, typename T>
inline SendBufferRef MakeSendBuffer(const T& msg)
{
    const uint32 bodySize  = static_cast<uint32>(msg.ByteSizeLong());
    const uint32 totalSize = sizeof(PacketHeader) + bodySize;
    SendBufferRef sendBuffer = GSendBufferManager->Open(totalSize);
    PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->GetBuffer());
    header->size = static_cast<uint16>(totalSize);
    header->type = static_cast<uint16>(PacketType);
    msg.SerializeToArray(sendBuffer->GetBuffer() + sizeof(PacketHeader), static_cast<int>(bodySize));
    sendBuffer->Close(totalSize);
    return sendBuffer;
}
