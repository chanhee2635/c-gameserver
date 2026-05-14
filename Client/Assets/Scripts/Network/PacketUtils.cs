using System;
using System.Buffers.Binary;
using Google.Protobuf;

public static class PacketUtils
{
    public static ArraySegment<byte> MakeSendBuffer(IMessage msg, ushort packetType)
    {
        int bodySize  = msg.CalculateSize();
        int totalSize = 4 + bodySize; 

        byte[] buf = new byte[totalSize];
        BinaryPrimitives.WriteUInt16LittleEndian(buf.AsSpan(0), (ushort)totalSize);
        BinaryPrimitives.WriteUInt16LittleEndian(buf.AsSpan(2), packetType);

        if (bodySize > 0)
            msg.WriteTo(buf.AsSpan(4));

        return new ArraySegment<byte>(buf, 0, totalSize);
    }
}
