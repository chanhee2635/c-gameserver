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

// �߻�Ŭ������ Session�� ��ü ����X, �������ܿ��� Session�� �ڽ� Ŭ���� ��ü�� �����ϴ� �Լ��� �����ϱ� ����
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
	// ���ο� ���� ��ü�� �����ϰ� IOCP�� ���
	SessionRef			CreateSession();
	// Ȱ��ȭ�� ������ ���� ���� ��Ͽ� �߰�
	void				AddSession(SessionRef session);
	void				ReleaseSession(SessionRef session);
	int32				GetCurrentSessionCount() { return _sessionCount.load(); }
	int32				GetAcceptCount() { return _config.acceptCount; }

public:
	ServiceType			GetServiceType() { return _type; }
	NetAddress			GetNetAddress() { return _netAddress; }
	IocpCoreRef&		GetIocpCore() { return _iocpCore; }
	const ServerConfig& GetConfig() { return _config; }
	
	void ForEachSession(const function<void(SessionRef)>& callback);

protected:
	USE_LOCK;

	ServiceType			_type;
	NetAddress			_netAddress = {};
	IocpCoreRef			_iocpCore;

	HashSet<SessionRef>	_sessions;  // HashSet�� Thread-Safe ���� �ʾ� MUTEX �ʿ�. (Red-Black Ʈ��)
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

	/*@brief ���� ������ Ŭ���̾�Ʈ ���� ������ �����Ѵ�.*/
	virtual bool	Start() override;
	/*@brief ���� ���񽺸� �ߴ��ϰ� ���� �ڿ��� �����Ѵ�.*/
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

	/*@brief �α��� ������ Ŭ���̾�Ʈ ���� ������ �����Ѵ�.*/
	virtual bool	Start() override;
	/*@brief �α��� ���񽺸� �ߴ��ϰ� ���� �ڿ��� �����Ѵ�.*/
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