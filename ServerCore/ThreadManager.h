#pragma once

#include <thread>
#include <functional>

/*----------------------
	Threadmanager
-----------------------*/

class ThreadManager
{
public:
	ThreadManager();
	~ThreadManager();

	/*
	* @brief 새로운 워커 스레드를 생성하고 실행을 관리
	* @param callback 스레드가 실행할 실제 로직
	*/
	void Launch(function<void(void)> callback);

	/* @brief 관리 중인 모든 스레드의 종료를 기다리고 자원을 정리 */
	void Join();

	/* TLS 초기화 */
	static void InitTLS();
	/* TLS에 할당된 자원들을 정리 */
	static void DestroyTLS();

	static void DoGlobalQueueWork();
	static void DoDBQueueWork();
	static void DistributeReservedJobs();

private:
	Mutex			_lock;
	vector<thread>	_threads;
};

