#pragma once
#include "NetAddress.h"
#include "IocpCore.h"
#include "Listener.h"
#include "ConfigManager.h"
#include <functional>

enum class ServiceType : uint8
{
	Login,
	Server,
	Client,
	Chat
};

/*-------------
	Service
--------------*/

// 추상클래스인 Session은 객체 생성X, 컨텐츠단에서 Session의 자식 클래스 객체를 생성하는 함수를 전달하기 위함
using SessionFactory = function<SessionRef(void)>;

class Service : public enable_shared_from_this<Service>
{
public:
	Service(ServiceType type, NetAddress address, IocpCoreRef core, SessionFactory factory);
	virtual ~Service();

	virtual bool		Start() abstract;
	bool				CanStart() { return _sessionFactory != nullptr; }

	virtual void		CloseService();
	void				SetSessionFactory(SessionFactory func) { _sessionFactory = func; }

	void				Broadcast(SendBufferRef sendBuffer);
	// 새로운 세션 객체를 생성하고 IOCP에 등록
	SessionRef			CreateSession();
	// 활성화된 세션을 서비스 관리 목록에 추가
	void				AddSession(SessionRef session);
	void				ReleaseSession(SessionRef session);
	int32				GetCurrentSessionCount() { return _sessionCount.load(); }
	int32				GetAcceptCount() { return _config.acceptCount; }

public:
	ServiceType			GetServiceType() { return _type; }
	NetAddress			GetNetAddress() { return _netAddress; }
	IocpCoreRef&		GetIocpCore() { return _iocpCore; }
	const ServerConfig& GetConfig() { return _config; }
	

protected:
	USE_LOCK;

	ServiceType			_type;
	NetAddress			_netAddress = {};
	IocpCoreRef			_iocpCore;

	set<SessionRef>		_sessions;  // set은 Thread-Safe 하지 않아 MUTEX 필요. (Red-Black 트리)
	atomic<int32>		_sessionCount = 0;
	SessionFactory		_sessionFactory;

	ServerConfig		_config;
};

/*-----------------
	ClientService
------------------*/

class ClientService : public Service
{
public:
	ClientService(NetAddress targetAddress, IocpCoreRef core, SessionFactory factory);
	virtual ~ClientService() {}

	virtual bool	Start() override;
};

/*-----------------
	ServerService
------------------*/

class ServerService : public Service
{
public:
	ServerService(NetAddress targetAddress, IocpCoreRef core, SessionFactory factory);
	virtual ~ServerService() {}

	/*@brief 게임 서비스의 클라이언트 접속 수락을 시작한다.*/
	virtual bool	Start() override;
	/*@brief 게임 서비스를 중단하고 관련 자원을 해제한다.*/
	virtual void	CloseService() override;

private:
	ListenerRef		_listener = nullptr;
};

/*----------------
	LoginService
-----------------*/

class LoginService : public Service
{
public:
	LoginService(NetAddress targetAddress, IocpCoreRef core, SessionFactory factory);
	virtual ~LoginService() {}

	/*@brief 로그인 서비스의 클라이언트 접속 수락을 시작한다.*/
	virtual bool	Start() override;
	/*@brief 로그인 서비스를 중단하고 관련 자원을 해제한다.*/
	virtual void	CloseService() override;

private:
	ListenerRef		_listener = nullptr;
};

/*---------------
	ChatService
-----------------*/

class ChatService : public Service
{
public:
	ChatService(NetAddress targetAddress, IocpCoreRef core, SessionFactory factory);
	virtual ~ChatService() {}

	virtual bool	Start() override;

	virtual void	CloseService() override;

private:
	ListenerRef		_listener = nullptr;
};