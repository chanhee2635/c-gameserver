#include "pch.h"
#include "PrometheusServer.h"
#include "GameMetrics.h"
#include "CoreGlobal.h"   // GLogicJobCount, GDBJobCount
#include <sstream>

PrometheusServer GPrometheusServer;

// ──────────────────────────────────────────────────────────
// Prometheus Exposition Format 헬퍼 매크로
// # HELP / # TYPE / 값 한 번에 출력
// ──────────────────────────────────────────────────────────
#define METRIC_GAUGE(oss, name, help, value) \
    (oss) << "# HELP " << (name) << " " << (help) << "\n"; \
    (oss) << "# TYPE " << (name) << " gauge\n"; \
    (oss) << (name) << " " << (value) << "\n\n"

#define METRIC_COUNTER(oss, name, help, value) \
    (oss) << "# HELP " << (name) << " " << (help) << "\n"; \
    (oss) << "# TYPE " << (name) << " counter\n"; \
    (oss) << (name) << " " << (value) << "\n\n"

std::string PrometheusServer::BuildMetrics()
{
    std::ostringstream oss;

    METRIC_GAUGE(oss, "game_server_uptime_seconds", "Server uptime in seconds", GMetrics.UptimeSeconds());
    METRIC_GAUGE(oss, "game_connected_sessions", "Current connected sessions", GMetrics.connectedSessions.load());
    METRIC_COUNTER(oss, "game_total_connections", "Total connection count", GMetrics.totalConnections.load());
    METRIC_COUNTER(oss, "game_total_disconnections", "Total disconnection count", GMetrics.totalDisconnections.load());
    METRIC_COUNTER(oss, "game_disconnect_recv0_total", "Disconnects caused by Recv 0 bytes", GMetrics.disconnectRecv0.load());
    METRIC_COUNTER(oss, "game_disconnect_send0_total", "Disconnects caused by Send 0 bytes", GMetrics.disconnectSend0.load());
    METRIC_COUNTER(oss, "game_disconnect_recv_overflow_total", "Disconnects caused by RecvBuffer overflow", GMetrics.disconnectRecvOverflow.load());
    METRIC_COUNTER(oss, "game_disconnect_wsa_error_total", "Disconnects caused by WSA error", GMetrics.disconnectHandleError.load());
    METRIC_COUNTER(oss, "game_packets_received_total", "Total packets received", GMetrics.totalPacketsReceived.load());
    METRIC_COUNTER(oss, "game_packets_sent_total", "Total packets sent", GMetrics.totalPacketsSent.load());
    METRIC_COUNTER(oss, "game_bytes_received_total", "Total bytes received", GMetrics.totalBytesReceived.load());
    METRIC_COUNTER(oss, "game_bytes_sent_total", "Total bytes sent", GMetrics.totalBytesSent.load());
    METRIC_COUNTER(oss, "game_invalid_packets_total", "Total invalid packets", GMetrics.invalidPackets.load());
    METRIC_GAUGE(oss, "game_active_players", "Active players in world", GMetrics.activePlayers.load());
    METRIC_GAUGE(oss, "game_active_monsters", "Active monsters in world", GMetrics.activeMonsters.load());
    METRIC_COUNTER(oss, "game_player_deaths_total", "Total player deaths", GMetrics.totalPlayerDeaths.load());
    METRIC_COUNTER(oss, "game_monster_deaths_total", "Total monster deaths", GMetrics.totalMonsterDeaths.load());
    METRIC_COUNTER(oss, "game_player_revives_total", "Total player revives", GMetrics.totalPlayerRevives.load());
    METRIC_COUNTER(oss, "game_zone_changes_total", "Total zone changes", GMetrics.totalZoneChanges.load());
    METRIC_GAUGE(oss, "game_logic_job_queue_size", "Current logic job queue size", GLogicJobCount.load());
    METRIC_GAUGE(oss, "game_db_job_queue_size", "Current DB job queue size", GDBJobCount.load());
    METRIC_COUNTER(oss, "game_db_query_success_total", "Total successful DB queries", GMetrics.dbQuerySuccess.load());
    METRIC_COUNTER(oss, "game_db_query_fail_total", "Total failed DB queries", GMetrics.dbQueryFail.load());
    METRIC_COUNTER(oss, "game_db_pool_empty_total", "Total times DB connection pool was empty", GMetrics.dbConnectionPoolEmpty.load());
    METRIC_COUNTER(oss, "game_send_buffer_chunk_alloc_total", "Total SendBufferChunk new allocations", GMetrics.sendBufferChunkAlloc.load());
    METRIC_COUNTER(oss, "game_send_buffer_chunk_reuse_total", "Total SendBufferChunk reuses from pool", GMetrics.sendBufferChunkReuse.load());
    METRIC_COUNTER(oss, "game_broadcasts_total", "Total BroadcastScene calls", GMetrics.totalBroadcasts.load());
    METRIC_COUNTER(oss, "game_broadcast_splits_total", "Total split broadcast calls", GMetrics.totalBroadcastSplits.load());

    return oss.str();
}

// ──────────────────────────────────────────────────────────
// HTTP 서버 루프
// ──────────────────────────────────────────────────────────
void PrometheusServer::Start(uint16_t port)
{
    _running = true;
    _thread = std::thread(&PrometheusServer::Run, this, port);
    _thread.detach();
}

void PrometheusServer::Stop()
{
    _running = false;
    if (_listenSocket != INVALID_SOCKET)
        ::closesocket(_listenSocket);
}

void PrometheusServer::Run(uint16_t port)
{
    _listenSocket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (_listenSocket == INVALID_SOCKET) return;

    int opt = 1;
    ::setsockopt(_listenSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    SOCKADDR_IN addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = ::htons(port);

    if (SOCKET_ERROR == ::bind(_listenSocket, (SOCKADDR*)&addr, sizeof(addr))) return;
    if (SOCKET_ERROR == ::listen(_listenSocket, 5)) return;

    while (_running)
    {
        // 타임아웃 설정 (select로 폴링하여 Stop() 시 블로킹 탈출)
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(_listenSocket, &readSet);
        timeval timeout{ 1, 0 };  // 1초

        int result = ::select(0, &readSet, nullptr, nullptr, &timeout);
        if (result <= 0) continue;

        SOCKET clientSock = ::accept(_listenSocket, nullptr, nullptr);
        if (clientSock == INVALID_SOCKET) break;

        // 요청 헤더 읽기 (내용은 무시, /metrics 여부만 확인해도 충분)
        char reqBuf[2048];
        ::recv(clientSock, reqBuf, sizeof(reqBuf) - 1, 0);

        std::string body = BuildMetrics();
        std::ostringstream response;
        response << "HTTP/1.1 200 OK\r\n";
        response << "Content-Type: text/plain; version=0.0.4; charset=utf-8\r\n";
        response << "Content-Length: " << body.size() << "\r\n";
        response << "Connection: close\r\n";
        response << "\r\n";
        response << body;

        std::string res = response.str();
        ::send(clientSock, res.c_str(), (int)res.size(), 0);
        ::closesocket(clientSock);
    }

    ::closesocket(_listenSocket);
    _listenSocket = INVALID_SOCKET;
}