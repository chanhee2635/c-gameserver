#pragma once
#include <atomic>
#include <string>
#include <chrono>

/*=======================================================================
    GameMetrics
    - ¸ðµç atomic Ä«¿îÅÍ´Â lock-free·Î µ¿ÀÛ (¸ÖÆ¼½º·¹µå ¾ÈÀü)
    - °¢ Ä«¿îÅÍ´Â PrometheusServer°¡ /metrics ¿£µåÆ÷ÀÎÆ®·Î ³ëÃâ
=======================================================================*/

struct GameMetrics
{
    // ¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡
    // [1] ³×Æ®¿öÅ© / ¼¼¼Ç
    // ¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡
    std::atomic<int32_t>  connectedSessions{ 0 };       // ÇöÀç ¿¬°áµÈ ÃÑ ¼¼¼Ç ¼ö
    std::atomic<int64_t>  totalConnections{ 0 };         // ´©Àû Á¢¼Ó ¼ö
    std::atomic<int64_t>  totalDisconnections{ 0 };      // ´©Àû Á¢¼Ó ÇØÁ¦ ¼ö
    std::atomic<int64_t>  disconnectRecv0{ 0 };          // Recv 0 bytes ·Î ²÷±ä È½¼ö
    std::atomic<int64_t>  disconnectSend0{ 0 };          // Send 0 bytes ·Î ²÷±ä È½¼ö
    std::atomic<int64_t>  disconnectRecvOverflow{ 0 };   // RecvBuffer ¿À¹öÇÃ·Î¿ì·Î ²÷±ä È½¼ö
    std::atomic<int64_t>  disconnectHandleError{ 0 };    // WSA ¿¡·¯·Î ²÷±ä È½¼ö

    // ¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡
    // [2] ÆÐÅ¶ Ã³¸®·®
    // ¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡
    std::atomic<int64_t>  totalPacketsReceived{ 0 };     // ´©Àû ¼ö½Å ÆÐÅ¶ ¼ö
    std::atomic<int64_t>  totalPacketsSent{ 0 };         // ´©Àû ¼Û½Å ÆÐÅ¶ ¼ö
    std::atomic<int64_t>  totalBytesReceived{ 0 };       // ´©Àû ¼ö½Å ¹ÙÀÌÆ®
    std::atomic<int64_t>  totalBytesSent{ 0 };           // ´©Àû ¼Û½Å ¹ÙÀÌÆ®
    std::atomic<int64_t>  invalidPackets{ 0 };           // À¯È¿ÇÏÁö ¾ÊÀº ÆÐÅ¶ ¼ö (Çì´õ ºÒ·® µî)

    // ¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡
    // [3] °ÔÀÓ ¿ÀºêÁ§Æ®
    // ¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡
    std::atomic<int32_t>  activePlayers{ 0 };            // °ÔÀÓ ¿ùµå¿¡ Á¸ÀçÇÏ´Â ÇÃ·¹ÀÌ¾î ¼ö
    std::atomic<int32_t>  activeMonsters{ 0 };           // °ÔÀÓ ¿ùµå¿¡ Á¸ÀçÇÏ´Â ¸ó½ºÅÍ ¼ö
    std::atomic<int64_t>  totalPlayerDeaths{ 0 };        // ´©Àû ÇÃ·¹ÀÌ¾î »ç¸Á ¼ö
    std::atomic<int64_t>  totalMonsterDeaths{ 0 };       // ´©Àû ¸ó½ºÅÍ »ç¸Á ¼ö
    std::atomic<int64_t>  totalPlayerRevives{ 0 };       // ´©Àû ÇÃ·¹ÀÌ¾î ºÎÈ° ¼ö
    std::atomic<int64_t>  totalZoneChanges{ 0 };         // ´©Àû Zone ÀÌµ¿ È½¼ö

    // ¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡
    // [4] JobQueue (·ÎÁ÷ ºÎÇÏ)
    // ¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡
    // GLogicJobCount, GDBJobCount ´Â CoreGlobal.h ¿¡ ÀÌ¹Ì ¼±¾ðµÊ
    // ¡æ BuildMetrics() ¿¡¼­ Á÷Á¢ ÂüÁ¶

    // ¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡
    // [5] DB
    // ¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡
    std::atomic<int64_t>  dbQuerySuccess{ 0 };           // ¼º°øÇÑ DB Äõ¸® ¼ö
    std::atomic<int64_t>  dbQueryFail{ 0 };              // ½ÇÆÐÇÑ DB Äõ¸® ¼ö
    std::atomic<int64_t>  dbConnectionPoolEmpty{ 0 };    // Ä¿³Ø¼Ç Ç®ÀÌ ºñ¾îÀÖ´ø È½¼ö

    // ¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡
    // [6] SendBuffer
    // ¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡
    std::atomic<int64_t>  sendBufferChunkAlloc{ 0 };     // SendBufferChunk ½Å±Ô ÇÒ´ç È½¼ö
    std::atomic<int64_t>  sendBufferChunkReuse{ 0 };     // SendBufferChunk Àç»ç¿ë È½¼ö

    // ¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡
    // [7] ºê·ÎµåÄ³½ºÆ®
    // ¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡
    std::atomic<int64_t>  totalBroadcasts{ 0 };          // BroadcastScene È£Ãâ È½¼ö
    std::atomic<int64_t>  totalBroadcastSplits{ 0 };     // ÆÐÅ¶ ºÐÇÒ ºê·ÎµåÄ³½ºÆ® È½¼ö

    // ¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡
    // [8] ¼­¹ö °¡µ¿ ½Ã°£
    // ¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡¦¡
    std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();

    int64_t UptimeSeconds() const
    {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count();
    }
};

extern GameMetrics GMetrics;