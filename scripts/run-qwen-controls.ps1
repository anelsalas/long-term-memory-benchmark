param([string]$Plan = 'configs/local-qwen-smoke.example.json',
      [string]$RunDirectory = ('runtime/controls-' + [guid]::NewGuid().ToString('N')))
$ErrorActionPreference = 'Stop'
New-Item -ItemType Directory -Path $RunDirectory | Out-Null
$base = Get-Content $Plan -Raw | ConvertFrom-Json
$base | Add-Member conditions @('no_memory','full_history','retrieved_evidence','known_evidence')
$base.provider_policy.max_requests = '4'
$base.provider_policy.max_tokens = '4096'
$base.source_revision = 'cf4f21f+four-controls-sha256:' + (Get-FileHash src/compare_main.cpp -Algorithm SHA256).Hash.ToLower()
$base.output_directory = 'artifacts'
$conditions = @('no_memory','full_history','retrieved_evidence','known_evidence')
foreach ($index in 0..19) {
  $base.seed = [string](42 + $index)
  $base.conditions = @(0..3 | ForEach-Object { $conditions[($_ + $index) % 4] })
  $path = "$RunDirectory/seed-$($base.seed).json"
  $base | ConvertTo-Json -Depth 12 | Set-Content $path
  $preflight = & ./build-vs2026/Release/ltm-compare.exe --config $path --dry-run
  if ($LASTEXITCODE -ne 0) { throw "Preflight failed seed $($base.seed): $preflight" }
}
foreach ($seed in 42..61) {
  $result = & ./build-vs2026/Release/ltm-compare.exe --config "$RunDirectory/seed-$seed.json" --execute
  $result | Set-Content "$RunDirectory/result-$seed.json"
  Write-Output "Seed ${seed}: $result"
  if ($LASTEXITCODE -ne 0) { throw "Execution incomplete at seed $seed" }
}
