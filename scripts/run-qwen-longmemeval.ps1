# Answers selected LongMemEval_S questions with the benchmark Qwen server, one
# ltm-compare run per question: question only, full history, BM25/dense/hybrid packed to a
# budget (dense and hybrid read the embedding cache written by ltm-retrieval-eval), and the
# labelled evidence alone. Each condition is dry-run first; one that does not fit
# the server's context is recorded as does_not_fit instead of being sent.
param(
    [string]$Singletons = 'runtime/retrieval-baselines/singletons/singletons.json',
    [string]$RunDirectory = 'runtime/qwen-longmemeval',
    [string]$Endpoint = 'http://127.0.0.1:8772/v1/chat/completions',
    [string]$Model = 'qwen3-0.6b-q8_0-32k',
    [int]$BudgetTokens = 6000,
    [int]$ContextTokens = 32768,
    [int]$OutputTokens = 1024,
    [string[]]$Conditions = @('no_memory', 'full_history', 'bm25_budget', 'dense_budget', 'hybrid_budget', 'known_evidence'),
    [string]$EmbeddingCache = 'runtime/embedding-cache/qwen3-embedding-0.6b-q8_0_last-pool-l2',
    [int]$Limit = 0,
    [switch]$DryRun  # report what fits without sending requests
)
$ErrorActionPreference = 'Stop'
New-Item -ItemType Directory -Force $RunDirectory | Out-Null
$selection = Get-Content $Singletons -Raw | ConvertFrom-Json
$questions = @($selection.questions)
if ($Limit -gt 0) { $questions = $questions | Select-Object -First $Limit }
$revision = (git rev-parse --short HEAD).Trim()

function New-Config([object]$question, [string[]]$conditions) {
    [ordered]@{
        schema_version = 1; source_revision = $revision; endpoint_url = $Endpoint
        execution_mode = 'local'; prompt_profile = 'standard'; include_fresh = $false
        conditions = $conditions; context_budget_tokens = [string]$BudgetTokens
        embedding_cache = (Resolve-Path $EmbeddingCache).Path
        reader = @{ id = $Model; version = $Model }; judge = @{ id = $Model; version = $Model }
        provider_policy = [ordered]@{
            max_requests = '20'; max_tokens = '2000000'; max_cost_nanos = '0'
            input_token_price_nanos = '0'; output_token_price_nanos = '0'; request_price_nanos = '0'
            max_prompt_bytes = '4000000'; max_response_bytes = '1000000'; max_attempts = 1
        }
        input_token_upper_bound = [string]($ContextTokens - $OutputTokens); context_token_limit = [string]$ContextTokens
        output_token_limit = [string]$OutputTokens; judge_output_token_limit = [string]$OutputTokens
        max_candidate_answer_bytes = '8192'; seed = '42'; top_k = 1; timeout_ms = 600000
        output_token_parameter = 'max_tokens'; temperature = 0; output_directory = 'artifacts'
        chat_framing_token_allowance = 256; tokenizer_bound_verified = $true
        public_case = [ordered]@{
            path = $question.path; question_id = $question.question_id
            source_uri = $selection.source_uri; revision = $selection.revision; sha256 = $question.sha256
        }
    }
}

$records = @()
foreach ($question in $questions) {
    $id = $question.question_id
    $fitting = @(); $notFitting = @()
    foreach ($condition in $Conditions) {
        $probe = Join-Path $RunDirectory "$id.$condition.probe.json"
        [IO.File]::WriteAllText((Join-Path (Get-Location) $probe), ((New-Config $question @($condition)) | ConvertTo-Json -Depth 6))
        $ErrorActionPreference = 'Continue'  # the expected stderr code line is not a script error
        $probeOutput = & ./build-vs2026/Release/ltm-compare.exe --config $probe --dry-run 2>$null | Out-String
        $ErrorActionPreference = 'Stop'
        if ($LASTEXITCODE -eq 0) { $fitting += $condition }
        elseif ($probeOutput -match '"stage":"prompt_preflight"') { $notFitting += $condition }
        else { throw "Dry run of $condition for $id failed: $probeOutput" }
        Remove-Item $probe
    }
    $result = $null
    if ($fitting.Count -and -not $DryRun) {
        $path = Join-Path $RunDirectory "$id.json"
        [IO.File]::WriteAllText((Join-Path (Get-Location) $path), ((New-Config $question $fitting) | ConvertTo-Json -Depth 6))
        $result = & ./build-vs2026/Release/ltm-compare.exe --config $path --execute | Out-String
        $result | Set-Content (Join-Path $RunDirectory "$id.result.json")
    }
    $records += [ordered]@{ question_id = $id; question_type = $question.question_type
        ran = $fitting; does_not_fit = $notFitting; summary = if ($result) { $result | ConvertFrom-Json } else { $null } }
    Write-Output "$id ran=[$($fitting -join ',')] does_not_fit=[$($notFitting -join ',')]"
}
[IO.File]::WriteAllText((Join-Path (Get-Location) (Join-Path $RunDirectory 'runs.json')), ($records | ConvertTo-Json -Depth 8))
