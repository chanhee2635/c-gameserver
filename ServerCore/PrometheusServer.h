#pragma once
#include <string>
#include <thread>
#include <atomic>

/*=======================================================================
    PrometheusServer
    - 별도 스레드에서 경량 HTTP 서버 실행
    - GET /metrics 요청에 Prometheus Exposition Format 텍스트 응답
    - WinSock2 기반 (기존 SocketUtils와 독립적으로 동작)
=======================================================================*/

class PrometheusServer
{
public:
    void    Start(uint16_t port = 8080);
    void    Stop();

private:
    void    Run(uint16_t port);
    std::string BuildMetrics();   // Prometheus exposition format 텍스트 생성

    std::thread         _thread;
    std::atomic<bool>   _running{ false };
    SOCKET              _listenSocket = INVALID_SOCKET;
};

extern PrometheusServer GPrometheusServer;