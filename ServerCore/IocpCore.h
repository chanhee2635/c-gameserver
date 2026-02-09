#pragma once

/*------------------
     IocpObject
------------------*/

/*
* @brief IOCP에 등록 가능한 모든 네트워크 객체의 최상위 부모 클래스
* @details 실제 핸들을 관리하는 래퍼 객체들이 공통적으로 구현해야 할 인터페이스를 정의한다.
*/
class IocpObject : public enable_shared_from_this<IocpObject> // 순환참조 방지(weak_ptr 사용)
{
public:
    // 상속 클래스에서 핸들(소켓)을 반환해야 함
    virtual HANDLE  GetHandle() abstract;
    // IOCP 완료 통지가 왔을 때 실행할 로직을 정의함
    virtual void    Dispatch(struct IocpEvent* iocpEvent, int32 numOfBytes = 0) abstract;
};

/*-----------------
     IocpCore 
-----------------*/

class IocpCore
{
public:
    IocpCore();
    ~IocpCore();

    HANDLE      GetHandle() { return _iocpHandle; }
    /*
    * @brief 소켓 핸들을 IOCP 커널 객체에 등록
    * @details 이 과정으로 핸들에서 발생하는 비동기 입출력 완료 신호가 IOCP 큐(Completion Queue)에 쌓여, 이후 Worker 스레드가 이를 감지한다.
    */
    bool        Register(IocpObjectRef iocpObject);
    /*
    * @brief IOCP 큐에서 완료된 I/O 이벤트를 꺼내어 해당 객체에 배분
    * @details 
    * 1. GetQueuedCompletionStatus는 비동기 작업이 완료될 때까지 스레드를 대기시킨다.
    * 2. 완료 통지가 오면 OVERLAPPED 구조체를 확장한 IocpEvent를 얻는다.
    * 3. iocpEvent->owner(IocpObject)를 통해 실질적인 로직을 실행한다.
    */
    bool        Dispatch(uint32 timeoutMs = INFINITE);

private:
    HANDLE      _iocpHandle;
};
