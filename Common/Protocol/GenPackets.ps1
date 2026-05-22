$protoDir    = $PSScriptRoot
$solutionDir = Resolve-Path "$protoDir\..\.."

$packetMap = @(
    @{ Proto = "Protocol"; Project = "GameServer";  Handler = "ClientPacketHandler"; SessionType = "GameSession";  Side = "Server" }
    @{ Proto = "Protocol"; Project = "DummyClient"; Handler = "ServerPacketHandler"; SessionType = "DummySession"; Side = "Client" }
)

function EnumToClass($enumName) {
    $parts = $enumName -split '_'
    return ($parts | ForEach-Object {
        $_.Substring(0,1).ToUpper() + $_.Substring(1).ToLower()
    }) -join ''
}

foreach ($info in $packetMap)
{
    $proto       = $info.Proto
    $protoFile   = Join-Path $protoDir "$proto.proto"
    $projectDir  = Join-Path $solutionDir $info.Project
    $outputDir   = Join-Path $projectDir "Protocol"
    $handlerName = $info.Handler
    $sessionType = $info.SessionType
    $sessionRef  = "${sessionType}Ref"
    $isClient    = ($info.Side -eq "Client")

    $authHelper = if (-not $isClient) { @"

    static bool IsAuthenticated(const ${sessionRef}& session)
    {
        if (session->GetDbId() == 0)
        {
            LOG_WARN(L"Unauthenticated session - packet blocked");
            session->Disconnect();
            return false;
        }
        return true;
    }
"@ } else { "" }

    if (-not (Test-Path $protoFile)) { Write-Warning "[$proto] not found, skipping"; continue }
    if (-not (Test-Path $projectDir)) { Write-Warning "[$proto] $projectDir not found, skipping"; continue }

    $lines   = Get-Content $protoFile
    $packets = [System.Collections.Generic.List[string]]::new()
    $pattern = if ($isClient) { '^\s+(S_\w+)\s*=\s*\d+' } else { '^\s+(C_\w+)\s*=\s*\d+' }

    foreach ($line in $lines) {
        if ($line -match $pattern) { $packets.Add($matches[1]) }
    }

    if ($packets.Count -eq 0) { Write-Warning "[$proto -> $($info.Project)] No packets found"; continue }

    Write-Host "[$proto -> $($info.Project)] $($packets.Count) packet(s)" -ForegroundColor Cyan

    if (-not (Test-Path $outputDir)) { New-Item -ItemType Directory -Path $outputDir | Out-Null }

    $onHandleDecls = ($packets | ForEach-Object {
        $cls = EnumToClass $_
        "    static bool OnHandle_$_($sessionRef session, const Protocol::${cls}& pkt);"
    }) -join "`n"

    $packetUtilsContent = @"
#pragma once
#include "Session.h"
#include "$proto.pb.h"

template<auto PacketType, typename T>
inline SendBufferRef MakeSendBuffer(const T& msg)
{
    const uint32 bodySize  = static_cast<uint32>(msg.ByteSizeLong());
    const uint32 totalSize = sizeof(PacketHeader) + bodySize;
    SendBufferRef sendBuffer = GSendBufferManager->Open(totalSize);
    PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->GetBuffer());
    header->size = static_cast<uint16>(totalSize);
    header->type = static_cast<uint16>(PacketType);
    msg.SerializeToArray(sendBuffer->GetBuffer() + sizeof(PacketHeader), static_cast<int>(bodySize));
    sendBuffer->Close(totalSize);
    return sendBuffer;
}
"@

    $packetUtilsContent | Out-File (Join-Path $outputDir "PacketUtils.h") -Encoding utf8
    Write-Host "  -> $($info.Project)\Protocol\PacketUtils.h" -ForegroundColor Green

    $headerContent = @"
#pragma once
#include "PacketUtils.h"
#include "${sessionType}.h"

class $handlerName
{
public:
    static bool Handle($sessionRef session, std::span<const BYTE> packet, uint16 type);

$onHandleDecls

private:$authHelper
    template<typename MsgType, bool(*OnHandle)($sessionRef, const MsgType&)>
    static bool HandlePacket($sessionRef session, std::span<const BYTE> packet)
    {
        MsgType pkt;
        if (!pkt.ParseFromArray(
                packet.data() + sizeof(PacketHeader),
                static_cast<int>(packet.size() - sizeof(PacketHeader))))
            return false;
        return OnHandle(session, pkt);
    }
};
"@

    $switchCases = ($packets | ForEach-Object {
        $cls = EnumToClass $_
        "    case Protocol::$_`: return HandlePacket<Protocol::$cls, OnHandle_$_>(session, packet);"
    }) -join "`n"

    $generatedContent = @"
// AUTO-GENERATED -- DO NOT EDIT MANUALLY
#include "pch.h"
#include "${handlerName}.h"

bool ${handlerName}::Handle($sessionRef session, std::span<const BYTE> packet, uint16 type)
{
    switch (type)
    {
$switchCases
    default:
        LOG_WARN(L"Unknown packet type=" + std::to_wstring(type));
        return false;
    }
}
"@

    $stubPath = Join-Path $projectDir "${handlerName}.cpp"
    if (-not (Test-Path $stubPath)) {
        $stubBodies = ($packets | ForEach-Object {
            $cls = EnumToClass $_
@"

bool ${handlerName}::OnHandle_$_($sessionRef session, const Protocol::${cls}& pkt)
{
    return true;
}
"@
        }) -join ""

        @"
#include "pch.h"
#include "Protocol/${handlerName}.h"
$stubBodies
"@ | Out-File $stubPath -Encoding utf8
        Write-Host "  Stub created: ${handlerName}.cpp" -ForegroundColor Yellow
    }

    $headerContent    | Out-File (Join-Path $outputDir "${handlerName}.h")             -Encoding utf8
    $generatedContent | Out-File (Join-Path $outputDir "${handlerName}_Generated.cpp") -Encoding utf8

    Write-Host "  -> $($info.Project)\Protocol\${handlerName}.h" -ForegroundColor Green
    Write-Host "  -> $($info.Project)\Protocol\${handlerName}_Generated.cpp" -ForegroundColor Green
}

# ─── Unity C# 자동 생성 (Game.proto 전용, Login 없음) ─────────────────────────

$unityNetDir = Join-Path $solutionDir "Client\Assets\Scripts\Network"
$unityPktDir = Join-Path $solutionDir "Client\Assets\Scripts\Packet"
$protoFile   = Join-Path $protoDir "Protocol.proto"

if (Test-Path $protoFile)
{
    $lines      = Get-Content $protoFile
    $sLines     = [System.Collections.Generic.List[string]]::new()  
    $cLines     = [System.Collections.Generic.List[string]]::new() 
    $stubMethods = [System.Collections.Generic.List[string]]::new()

    foreach ($line in $lines)
    {
        if ($line -match '^\s+(S_[A-Z0-9_]+)\s*=\s*\d+')
        {
            $raw = $matches[1]                          # S_PLAYER_LIST
            $cls = EnumToClass $raw                     # SPlayerList
            $id  = "(ushort)Protocol.MsgId.$cls"
            $sLines.Add("        AddHandler<Protocol.$cls>($id, PacketHandler.${cls}Handler);")
            $stubMethods.Add("    public static void ${cls}Handler(Protocol.$cls pkt) { }")
        }
        if ($line -match '^\s+(C_[A-Z0-9_]+)\s*=\s*\d+')
        {
            $raw = $matches[1]                          # C_AUTH_TOKEN
            $cls = EnumToClass $raw                     # CAuthToken
            $id  = "(ushort)Protocol.MsgId.$cls"
            $cLines.Add("        { typeof(Protocol.$cls), $id },")
        }
    }

    # PacketManager_Generated.cs ← 매번 덮어씀
    $genCs = @"
using System;
using System.Collections.Generic;

public sealed partial class PacketManager
{
    partial void Register()
    {
$($sLines -join "`n")
    }
}

public static class PacketIdMapper
{
    static readonly Dictionary<Type, ushort> _map = new()
    {
$($cLines -join "`n")
    };
    public static ushort GetId(Type t) => _map.TryGetValue(t, out var id) ? id : (ushort)0;
}
"@
    if (-not (Test-Path $unityNetDir)) { New-Item -ItemType Directory $unityNetDir | Out-Null }
    $genCs | Out-File (Join-Path $unityNetDir "PacketManager_Generated.cs") -Encoding utf8
    Write-Host "  -> Network\PacketManager_Generated.cs" -ForegroundColor Green

    # PacketHandler.Game.cs ← 없을 때만 생성 (이후 직접 관리)
    $stubPath = Join-Path $unityPktDir "PacketHandler.Game.cs"
    if (-not (Test-Path $stubPath))
    {
        @"

public static partial class PacketHandler
{
$($stubMethods -join "`n")
}
"@ | Out-File $stubPath -Encoding utf8
        Write-Host "  Stub created: PacketHandler.Game.cs" -ForegroundColor Yellow
    }
}