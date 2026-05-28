$protoc         = "C:\Projects\vcpkg\installed\x64-windows\tools\protobuf\protoc.exe"
$protoDir       = $PSScriptRoot
$solutionDir    = Resolve-Path "$protoDir\..\.."
$unityPacketDir = Join-Path $solutionDir "Client\Assets\Scripts\Packet"

if (-not (Test-Path $protoc)) {
    Write-Error "protoc.exe not found: $protoc"
    exit 1
}

# Common 삭제 — Game.proto 하나로 통합
$deployMap = @{
    "Protocol" = @("GameServer", "DummyClient")
    "GatePacket" = @("GateServer", "DummyClient") 
}

$unityProtos = @("Protocol", "GatePacket")

# ─── C++ 생성 ─────────────────────────────────────────────────────────────────

$generated = @()

foreach ($proto in $deployMap.Keys)
{
    $protoFile = Join-Path $protoDir "$proto.proto"
    $pbHeader  = Join-Path $protoDir "$proto.pb.h"

    if ((Test-Path $pbHeader) -and
        (Get-Item $pbHeader).LastWriteTime -ge (Get-Item $protoFile).LastWriteTime)
    {
        Write-Host "[$proto C++] Up to date, skipping" -ForegroundColor Gray
        continue
    }

    Write-Host "[$proto C++] Generating..." -ForegroundColor Cyan
    & $protoc "--proto_path=$protoDir" "--cpp_out=$protoDir" "$protoFile"

    if ($LASTEXITCODE -ne 0) {
        Write-Error "[$proto C++] Generation failed"
        exit 1
    }

    $generated += $proto
}

foreach ($proto in $generated)
{
    foreach ($project in $deployMap[$proto])
    {
        $destDir = Join-Path (Join-Path $solutionDir $project) "Protocol"

        if (-not (Test-Path $destDir)) {
            New-Item -ItemType Directory -Path $destDir | Out-Null
        }

        Copy-Item "$protoDir\$proto.pb.h"  $destDir -Force
        Copy-Item "$protoDir\$proto.pb.cc" $destDir -Force

        Write-Host "  -> $project\Protocol" -ForegroundColor Green
    }

    Remove-Item "$protoDir\$proto.pb.h"  -Force
    Remove-Item "$protoDir\$proto.pb.cc" -Force
}

# ─── C# Unity 생성 ────────────────────────────────────────────────────────────

if (-not (Test-Path $unityPacketDir)) {
    New-Item -ItemType Directory -Path $unityPacketDir | Out-Null
}

foreach ($proto in $unityProtos)
{
    $protoFile = Join-Path $protoDir "$proto.proto"
    $csFile    = Join-Path $unityPacketDir "$proto.cs"

    if (-not (Test-Path $protoFile)) {
        Write-Warning "[$proto C#] $protoFile not found, skipping"
        continue
    }

    if ((Test-Path $csFile) -and
        (Get-Item $csFile).LastWriteTime -ge (Get-Item $protoFile).LastWriteTime)
    {
        Write-Host "[$proto C#] Up to date, skipping" -ForegroundColor Gray
        continue
    }

    Write-Host "[$proto C#] Generating Unity C#..." -ForegroundColor Cyan
    & $protoc "--proto_path=$protoDir" "--csharp_out=$unityPacketDir" "$protoFile"

    if ($LASTEXITCODE -ne 0) {
        Write-Error "[$proto C#] C# generation failed"
        exit 1
    }

    Write-Host "  -> Client\Assets\Scripts\Packet\$proto.cs" -ForegroundColor Green
}

Write-Host "`nDone." -ForegroundColor Yellow