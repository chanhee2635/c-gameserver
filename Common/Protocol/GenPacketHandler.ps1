# GenPacketHandler.ps1
# Parses S_ packet types from Game.proto and generates PacketHandler.cs
# Called by: GenCSharp.bat / Unity Editor (PacketHandlerGenerator.cs)

param(
    [string]$ProtoPath  = "$PSScriptRoot\Game.proto",
    [string]$OutputPath = "$PSScriptRoot\..\..\Client\Assets\Scripts\Network\PacketHandler.cs"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

# --------------------------------------------------------------------------
# Parse proto
# --------------------------------------------------------------------------
$proto = [System.IO.File]::ReadAllText($ProtoPath, [System.Text.Encoding]::UTF8)

$enumMatch = [regex]::Match(
    $proto,
    'enum\s+GamePacketType\s*\{([^}]*)\}',
    [System.Text.RegularExpressions.RegexOptions]::Singleline
)
if (-not $enumMatch.Success) {
    Write-Error "[GenPacketHandler] GamePacketType enum not found: $ProtoPath"
    exit 1
}

$rawMatches = [regex]::Matches(
    $enumMatch.Groups[1].Value,
    '^\s*(S_[A-Z0-9_]+)\s*=\s*\d+',
    [System.Text.RegularExpressions.RegexOptions]::Multiline
)

if ($rawMatches.Count -eq 0) {
    Write-Error '[GenPacketHandler] No S_ packets found'
    exit 1
}

# S_PLAYER_DATA -> SPlayerData
function ToPascal([string]$snake) {
    ($snake -split '_' | ForEach-Object {
        $_.Substring(0,1).ToUpperInvariant() + $_.Substring(1).ToLowerInvariant()
    }) -join ''
}

$packets = $rawMatches | ForEach-Object {
    $raw = $_.Groups[1].Value
    [PSCustomObject]@{ Raw = $raw; Pascal = (ToPascal $raw) }
}

# --------------------------------------------------------------------------
# Build source
# Use $lines.Add() (expression/method mode) instead of a custom function
# to avoid PowerShell 5.1 treating '<' as a redirect operator in arg mode.
# --------------------------------------------------------------------------
$lines = [System.Collections.Generic.List[string]]::new()

$lines.Add('// AUTO-GENERATED - DO NOT EDIT MANUALLY')
$lines.Add('//   Source : Common/Protocol/Game.proto')
$lines.Add('//   Regen  : GenCSharp.bat  |  Unity: Tools > Network > Generate PacketHandler')
$lines.Add('using System;')
$lines.Add('using System.Collections.Generic;')
$lines.Add('using UnityEngine;')
$lines.Add('')
$lines.Add('namespace Network')
$lines.Add('{')
$lines.Add('    // Dispatches received packets to typed handlers (called on main thread).')
$lines.Add('    // Call Register() once, then Flush() every Update.')
$lines.Add('    // Subscribe to OnS* events in GameManager / UI.')
$lines.Add('    public static class PacketHandler')
$lines.Add('    {')

# Events
$lines.Add('        // -- Server -> Client events -----------------------------------------------')
foreach ($p in $packets) {
    $lines.Add("        public static event Action<Protocol.$($p.Pascal)> On$($p.Pascal);")
}
$lines.Add('')

# Dispatch table
$lines.Add('        // -- Dispatch table ---------------------------------------------------------')
$lines.Add('        private static readonly Dictionary<ushort, Action<byte[]>> _table = new();')
$lines.Add('        private const int HeaderSize = PacketHeader.HeaderSize;')
$lines.Add('')

# Static reset for Unity Editor play-mode re-entry
$lines.Add('        // Unity Editor: reset static state when re-entering Play mode')
$lines.Add('        [RuntimeInitializeOnLoadMethod(RuntimeInitializeLoadType.SubsystemRegistration)]')
$lines.Add('        private static void ResetStaticState()')
$lines.Add('        {')
$lines.Add('            _table.Clear();')
foreach ($p in $packets) {
    $lines.Add("            On$($p.Pascal) = null;")
}
$lines.Add('        }')
$lines.Add('')

# Parser cache
$lines.Add('        // -- Parser cache (created once per type, guaranteed by CLR) ---------------')
$lines.Add('        // Nested generic static class: O(1) access without Dictionary lookup or cast')
$lines.Add('        private static class ParserCache<T> where T : Google.Protobuf.IMessage<T>, new()')
$lines.Add('        {')
$lines.Add('            public static readonly Google.Protobuf.MessageParser<T> Value =')
$lines.Add('                new Google.Protobuf.MessageParser<T>(() => new T());')
$lines.Add('        }')
$lines.Add('')
$lines.Add('        private static T Parse<T>(byte[] data) where T : Google.Protobuf.IMessage<T>, new()')
$lines.Add('            => ParserCache<T>.Value.ParseFrom(data, HeaderSize, data.Length - HeaderSize);')
$lines.Add('')

# Register
$lines.Add('        // -- Registration -----------------------------------------------------------')
$lines.Add('        public static void Register()')
$lines.Add('        {')
$lines.Add('            _table.Clear();')
foreach ($p in $packets) {
    $lines.Add("            Add(Protocol.GamePacketType.$($p.Pascal), Handle_$($p.Raw));")
}
$lines.Add('        }')
$lines.Add('')

# Flush  — the $ in $"..." must be a literal dollar sign in generated C#.
# Build the warning line via concatenation to keep it unambiguous.
$warnLine = '                    Debug.LogWarning(' + '$"[PacketHandler] Unknown packet type: {pkt.Type}");'
$lines.Add('        // Call every Update on the main thread to process all queued packets')
$lines.Add('        public static void Flush()')
$lines.Add('        {')
$lines.Add('            PacketQueue.Instance.Flush(pkt =>')
$lines.Add('            {')
$lines.Add('                if (_table.TryGetValue(pkt.Type, out Action<byte[]> handler))')
$lines.Add('                    handler(pkt.Data);')
$lines.Add('                else')
$lines.Add($warnLine)
$lines.Add('            });')
$lines.Add('        }')
$lines.Add('')

# Add helper
$lines.Add('        private static void Add(Protocol.GamePacketType type, Action<byte[]> handler)')
$lines.Add('            => _table[(ushort)type] = handler;')
$lines.Add('')

# Handlers
$lines.Add('        // -- Handlers ---------------------------------------------------------------')
foreach ($p in $packets) {
    $lines.Add("        private static void Handle_$($p.Raw)(byte[] data)")
    $lines.Add("            => On$($p.Pascal)?.Invoke(Parse<Protocol.$($p.Pascal)>(data));")
    $lines.Add('')
}

$lines.Add('    }')
$lines.Add('}')

# --------------------------------------------------------------------------
# Write output (UTF-8 without BOM)
# --------------------------------------------------------------------------
$outputFull = [System.IO.Path]::GetFullPath($OutputPath)
$utf8NoBom  = New-Object System.Text.UTF8Encoding $false
[System.IO.File]::WriteAllLines($outputFull, $lines, $utf8NoBom)

Write-Host "[GenPacketHandler] Done -- $($packets.Count) S_ packets -> $outputFull"
