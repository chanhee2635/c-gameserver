#pragma once

/*-------------------
     NetAddress
-------------------*/

class NetAddress
{
public:
    NetAddress() = default;
    NetAddress(SOCKADDR_IN sockAddr);
    NetAddress(wstring ip, uint16 port);

    const SOCKADDR_IN&  GetSockAddr()   const { return _sockAddr; }
    wstring             GetIpAddress()  const;
    uint16              GetPort()       const { return ::ntohs(_sockAddr.sin_port); }

    static IN_ADDR Ip2Address(const WCHAR* ip);

private:
    SOCKADDR_IN _sockAddr = {};
};

