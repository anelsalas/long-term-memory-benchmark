param(
    [Parameter(Mandatory)] [int] $EmbeddingPid,
    [Parameter(Mandatory)] [int] $BridgePid,
    [Parameter(Mandatory)] [int] $SidecarPid,
    [Parameter(Mandatory)] [int] $RunnerPid,
    [Parameter(Mandatory)] [string] $Output,
    [ValidateRange(1, 60)] [int] $IntervalSeconds = 5
)

$ErrorActionPreference = 'Stop'
$roles = [ordered]@{ embedding = $EmbeddingPid; bridge = $BridgePid; sidecar = $SidecarPid; runner = $RunnerPid }
$identities = @{}
foreach ($role in $roles.Keys) {
    $process = Get-Process -Id $roles[$role] -ErrorAction Stop
    $identities[$role] = $process.StartTime.ToUniversalTime().ToString('o')
}
$stream = [IO.File]::Open($Output, [IO.FileMode]::CreateNew, [IO.FileAccess]::Write, [IO.FileShare]::Read)
$writer = [IO.StreamWriter]::new($stream, [Text.UTF8Encoding]::new($false))
try {
    while ($true) {
        $sample = [ordered]@{ timestamp_utc = [DateTime]::UtcNow.ToString('o'); processes = [ordered]@{} }
        $runnerAlive = $false
        foreach ($role in $roles.Keys) {
            $process = Get-Process -Id $roles[$role] -ErrorAction SilentlyContinue
            if ($null -eq $process -or $process.StartTime.ToUniversalTime().ToString('o') -ne $identities[$role]) {
                $sample.processes[$role] = $null
                continue
            }
            if ($role -eq 'runner') { $runnerAlive = $true }
            $sample.processes[$role] = [ordered]@{
                cpu_total_seconds = $process.CPU
                working_set_bytes = $process.WorkingSet64
                private_bytes = $process.PrivateMemorySize64
            }
        }
        $writer.WriteLine(($sample | ConvertTo-Json -Depth 5 -Compress))
        $writer.Flush()
        if (-not $runnerAlive) { break }
        Start-Sleep -Seconds $IntervalSeconds
    }
} finally {
    $writer.Dispose()
}
