#include "pch.h"
#include "Service.h"
#include "Session.h"
#include "Listener.h"
#include "ConfigManager.h"

/*-------------
	Service
--------------*/

Service::Service(ServiceType type, NetAddress address, IocpCoreRef core, SessionFactory factory)
	: _type(type), _netAddress(address), _iocpCore(core), _sessionFactory(factory)
{

}

Service::~Service()
{
}

void Service::CloseService()
{
	// 서비스의 공통 자원 해제
}

void Service::Broadcast(SendBufferRef sendBuffer)
{
	WRITE_LOCK;
	for (const auto& session : _sessions)
	{
		session->Send(sendBuffer);
	}
}

SessionRef Service::CreateSession()
{
	SessionRef session = _sessionFactory();  
	session->SetService(shared_from_this());

	if (_iocpCore->Register(session) == false)
		return nullptr;

	return session;
}

void Service::AddSession(SessionRef session)
{
	WRITE_LOCK;

	// 최대 접속 수 초과 시 X
	if (_sessionCount >= _config.maxSessionCount)
	{
		session->Disconnect(L"MaxSessionCount Exceeded");
		return;
	}

	_sessionCount.fetch_add(1);
	_sessions.insert(session); 
}

/* 
* 엔진단에서의 Session 삭제
*/
void Service::ReleaseSession(SessionRef session)
{
	WRITE_LOCK;
	ASSERT_CRASH(_sessions.erase(session) != 0);
	_sessionCount.fetch_sub(1);
}

/*-----------------
	ClientService
------------------*/

ClientService::ClientService(NetAddress targetAddress, IocpCoreRef core, SessionFactory factory)
	: Service(ServiceType::Client, targetAddress, core, factory)
{
}

bool ClientService::Start()
{
	if (CanStart() == false) return false;

	_config = GConfigManager->GetGame();


	return true;
}

/*-----------------
	ServerService
------------------*/

ServerService::ServerService(NetAddress address, IocpCoreRef core, SessionFactory factory)
	: Service(ServiceType::Server, address, core, factory)
{
}

bool ServerService::Start()
{
	if (CanStart() == false) return false;

	_config = GConfigManager->GetGame();

	_listener = MakeShared<Listener>();
	if (_listener == nullptr) return false;

	ServerServiceRef service = static_pointer_cast<ServerService>(shared_from_this());
	if (_listener->StartAccept(service) == false) return false;

	return true;
}

void ServerService::CloseService()
{
	// TODO
	_listener->CloseSocket();

	Service::CloseService();
}

/*----------------
	LoginService
-----------------*/

LoginService::LoginService(NetAddress address, IocpCoreRef core, SessionFactory factory)
	: Service(ServiceType::Login, address, core, factory)
{
}

bool LoginService::Start()
{
	if (CanStart() == false) return false;

	_config = GConfigManager->GetLogin();

	// 접속 수락을 담당할 리스너 객체 생성
	_listener = MakeShared<Listener>();
	if (_listener == nullptr) return false;

	// 리스너에게 나를 전달하며 비동기 Accept를 요청
	LoginServiceRef service = static_pointer_cast<LoginService>(shared_from_this());
	if (_listener->StartAccept(service) == false) return false;

	return true;
}

void LoginService::CloseService()
{
	_listener->CloseSocket();

	Service::CloseService();
}

/*----------------
	ChatService
-----------------*/

ChatService::ChatService(NetAddress address, IocpCoreRef core, SessionFactory factory)
	: Service(ServiceType::Chat, address, core, factory)
{
}

bool ChatService::Start()
{
	if (CanStart() == false) return false;

	_config = GConfigManager->GetChat();

	_listener = MakeShared<Listener>();
	if (_listener == nullptr) return false;

	ChatServiceRef service = static_pointer_cast<ChatService>(shared_from_this());
	if (_listener->StartAccept(service) == false) return false;

	return true;
}

void ChatService::CloseService()
{
	_listener->CloseSocket();

	Service::CloseService();
}