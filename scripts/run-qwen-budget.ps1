# Runs the fresh correction histories with distractors through local Qwen in four
# conditions: question only, keyword top-1, BM25 packed to a token budget, and the
# correction alone. Full history does not fit the local 512-byte prompt and is not run.
param(
    [string]$RunDirectory = 'runtime/qwen-budget',
    [int[]]$Seeds = 42..61,
    [int]$Distractors = 6,
    [int]$BudgetTokens = 50
)
$ErrorActionPreference = 'Stop'
New-Item -ItemType Directory -Force $RunDirectory | Out-Null
$conditions = @('no_memory', 'retrieved_evidence', 'bm25_budget', 'known_evidence')
$revision = (git rev-parse --short HEAD).Trim()
foreach ($seed in $Seeds) {
    $config = Get-Content 'configs/local-qwen-500.example.json' -Raw | ConvertFrom-Json
    $index = $seed - $Seeds[0]
    $config.source_revision = $revision
    $config.seed = [string]$seed
    $config.fresh_distractors = $Distractors
    $config.provider_policy.max_requests = '4'
    $config.provider_policy.max_tokens = '4096'
    $config.output_directory = 'artifacts'
    # Rotate condition order per seed, as the earlier controls run did.
    $config | Add-Member conditions @(0..3 | ForEach-Object { $conditions[($_ + $index) % 4] })
    $config | Add-Member context_budget_tokens ([string]$BudgetTokens)
    $path = Join-Path $RunDirectory "seed-$seed.json"
    [IO.File]::WriteAllText((Join-Path (Get-Location) $path), ($config | ConvertTo-Json -Depth 6))
    $result = & ./build-vs2026/Release/ltm-compare.exe --config $path --execute
    $result | Set-Content (Join-Path $RunDirectory "result-$seed.json")
    Write-Output "Seed ${seed}: $result"
    if ($LASTEXITCODE -ne 0) { throw "Execution incomplete at seed $seed" }
}
