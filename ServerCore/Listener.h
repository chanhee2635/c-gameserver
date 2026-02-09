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
    // 외부에서 사용
    /*
    * @brief 수신 대기 소켓을 초기화하고 비동기 접속 수락 요청을 등록
    * @details 소켓 생성 및 IOCP 등록, 소켓 옵션을 설정하고, 최대 세션 수만큼 커널에 AcceptEx 요청을 한다.
    */
    bool                    StartAccept(ServiceRef service);
    void                    CloseSocket();

public:
    // 인터페이스 구현
    virtual HANDLE          GetHandle() override;
    /*
    * @brief IOCP로부터 완료 통보를 받은 Accept 이벤트 처리
    * @param iocpEvent 완료된 Accept 작업의 정보를 담고 있는 이벤트 객체
    * @param numOfByte AcceptEx에서는 통상적으로 수신한 첫 데이터 크기 (현재 설정 0)
    */
    virtual void            Dispatch(struct IocpEvent* iocpEvent, int32 numOfByte = 0) override;

private:
    // 수신 관련
    /*
    * @brief 비동기 접속 수락(AcceptEx)을 커널에 등록
    */
    void                    RegisterAccept(IocpEvent* acceptEvent);
    /*
    * @brief 완료된 Accept 이벤트를 처리하여 세션을 활성화
    */
    void                    ProcessAccept(IocpEvent* acceptEvent);

protected:
    SOCKET              _socket = INVALID_SOCKET;
    vector<IocpEvent*>  _acceptEvents;
    ServiceRef          _service;
};