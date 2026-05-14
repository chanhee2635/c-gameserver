#pragma once
#include "IocpCore.h"
#include "IocpEvent.h"
#include "RecvBuffer.h"

class Session : public IocpObject
{
private:
	class RecvEvent : public IocpEvent
	{
	public:
		RecvEvent() : IocpEvent(IocpEventType::Recv) {}
	};
	class SendEvent : public IocpEvent
	{
	public:
		SendEvent() : IocpEvent(IocpEventType::Send) {}
	};
	class ConnectEvent : public IocpEvent
	{
	public:
		ConnectEvent() : IocpEvent(IocpEventType::Connect) {}
	};

public:
	Session();
	virtual ~Session();

	void	   SetNetAddress(const NetAddress& address) { _address = address; }
	void	   SetService(const ServiceRef& service) { _service = service; }
	SessionRef GetRef() { return std::static_pointer_cast<Session>(shared_from_this()); }
	NetAddress GetNetAddress() const { return _address; }
	SOCKET	   GetSocket()	   const { return _socket; }
	ServiceRef GetService()	   const { return _service.lock(); }
	bool	   IsConnected()   const { return _connected; }

	void Send(SendBufferRef sendBuffer);
	void ProcessConnect();
	void Disconnect();
	void RegisterConnect(const NetAddress& address);

	virtual HANDLE GetHandle() const override { return reinterpret_cast<HANDLE>(_socket); }
	virtual void   Dispatch(IocpEvent* iocpEvent, int32 numOfBytes = 0) override;

protected:
	virtual void  OnConnected() {}
	virtual void  OnDisconnected() {}
	virtual int32 OnRecv(const BYTE* buffer, uint32 len) { return static_cast<int32>(len); }
	virtual void  OnSend(uint32 len) {}

private:
	void RegisterRecv();
	void ProcessRecv(int32 numOfBytes);
	void RegisterSend();
	void ProcessSend(int32 numOfBytes);

	SOCKET			_socket = INVALID_SOCKET;
	NetAddress		_address = {};
	Atomic<bool>	_connected = false;
	Atomic<bool>	_pendingClose = false;
	ServiceWeakRef	_service;

	RecvEvent	 _recvEvent;
	SendEvent	 _sendEvent;
	ConnectEvent _connectEvent;

	RecvBuffer			  _recvBuffer;
	Mutex				  _sendLock;
	Vector<SendBufferRef> _sendQueue;
	Vector<SendBufferRef> _sendPendingList;
	bool				  _isSendRegistered = false;
};

struct PacketHeader
{
	uint16 size;
	uint16 type;
};

class PacketSession : public Session
{
public:
	virtual int32 OnRecv(const BYTE* buffer, uint32 len) final;
	virtual void  OnRecvPacket(std::span<const BYTE> packet, uint16 type) = 0;

private:
	static constexpr uint32 MAX_PACKET_SIZE = Config::Session::MAX_PACKET_SIZE;
};