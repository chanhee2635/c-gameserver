#pragma once
#include "IocpCore.h"
#include "NetAddress.h"

/*---------------
     Listener
---------------*/

class Listener : public IocpObject
{
public:
    Listener() = default;
    ~Listener();

public:
    /*
    * @brief 서버 리스닝 소켓을 초기화하고 비동기 Accept 대기 요청을 건다
    */
    bool                    StartAccept(ServiceRef service);
    void                    CloseSocket();

public:
    virtual HANDLE          GetHandle() override;
    virtual void            Dispatch(struct IocpEvent* iocpEvent, int32 numOfByte = 0) override;

private:
    /*
    * @brief 비동기 AcceptEx 를 큐에 등록
    */
    void                    RegisterAccept(AcceptEvent* acceptEvent);
    /*
    * @brief 완료된 Accept 이벤트를 처리하여 세션을 활성화
    */
    void                    ProcessAccept(AcceptEvent* acceptEvent);

protected:
    SOCKET              _socket = INVALID_SOCKET;
    // AcceptEvent: 값 멤버로 보유 (IocpEvent* 힙 할당 제거)
    // → StartAccept 에서 acceptCount 크기로 resize 후 고정
    Vector<AcceptEvent*>  _acceptEvents;
    ServiceRef          _service;
};
