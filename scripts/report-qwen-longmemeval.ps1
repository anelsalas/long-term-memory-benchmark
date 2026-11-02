# Summarizes runtime/qwen-longmemeval into a public record without dialogue text:
# judged correctness, whether labelled evidence was in the reader's context, real
# input-token counts and the source revision of each run. Answers stay in runtime.
param(
    [string]$RunDirectory = 'runtime/qwen-longmemeval',
    [string]$Singletons = 'runtime/retrieval-baselines/singletons/singletons.json',
    [string]$Output = 'reports/2026-11-01-qwen-longmemeval.json'
)
$ErrorActionPreference = 'Stop'
$types = @{}
foreach ($q in (Get-Content $Singletons -Raw | ConvertFrom-Json).questions) { $types[$q.question_id] = $q.question_type }

# Labelled evidence for a question: the known_evidence condition's sources in any
# run of that question (per-condition runs keep it in a separate run).
$knownCache = @{}
function ltm_known_evidence([string]$id, [string]$dir) {
    if ($knownCache.ContainsKey($id)) { return $knownCache[$id] }
    $sources = @()
    foreach ($candidate in @((Join-Path $RunDirectory "$id.result.json"), (Join-Path $RunDirectory "$id.known_evidence.result.json"))) {
        if (!(Test-Path $candidate)) { continue }
        $runDir = (Get-Content $candidate -Raw | ConvertFrom-Json).artifact_directory
        $plan = Get-Content (Join-Path $runDir 'plan.json') -Raw | ConvertFrom-Json
        $sources += @(($plan.cases | Where-Object condition -eq 'known_evidence').source_ids)
    }
    $knownCache[$id] = $sources
    return $sources
}

$rows = @(); $failed = @()
foreach ($file in Get-ChildItem $RunDirectory -Filter '*.result.json' | Sort-Object Name) {
    $name = $file.Name -replace '\.result\.json$', ''
    $id = $name.Split('.')[0]  # per-condition runs are named <id>.<condition>
    $summary = Get-Content $file.FullName -Raw | ConvertFrom-Json
    $dir = $summary.artifact_directory
    $config = Get-Content (Join-Path $RunDirectory "$name.json") -Raw | ConvertFrom-Json
    $plan = Get-Content (Join-Path $dir 'plan.json') -Raw | ConvertFrom-Json
    $records = (Get-Content (Join-Path $dir 'results.json') -Raw | ConvertFrom-Json).records
    $loaded = ltm_known_evidence $id $dir
    $known = @($loaded)
    for ($i = 0; $i -lt $plan.cases.Count; $i++) {
        $case = $plan.cases[$i]
        $record = $records | Where-Object case_id -eq $case.case_id
        $result = Get-Content (Join-Path $dir "case-$i.result.json") -Raw | ConvertFrom-Json
        $verdict = if ($result.judge) { ([string]$result.judge.attempts[-1].reply).Trim().ToLower() } else { $null }
        $rows += [pscustomobject]@{
            question_id = $id; question_type = $types[$id]; abstention = $id.EndsWith('_abs')
            condition = $case.condition; status = $record.status; judged_correct = [bool]$record.correct
            judge_verdict = $verdict
            evidence_in_context = if ($known.Count) { @($case.source_ids | Where-Object { $known -contains $_ }).Count -gt 0 } else { $null }
            input_tokens = $result.attempts[-1].usage.input_tokens; output_tokens = $result.attempts[-1].usage.output_tokens
            prompt_bytes = $case.prompt_bytes; reader_wall_ms = [math]::Round($result.reader_wall_ms)
            source_revision = $config.source_revision; output_token_limit = [int]$config.output_token_limit
        }
    }
}
foreach ($file in Get-ChildItem $RunDirectory -Filter '*.run.json') {
    $run = Get-Content $file.FullName -Raw | ConvertFrom-Json
    if ($run.error_stage -and -not (Test-Path (Join-Path $RunDirectory "$($run.question_id).result.json"))) {
        $failed += [pscustomobject]@{ question_id = $run.question_id; error_stage = $run.error_stage }
    }
}
$order = @('no_memory', 'bm25_budget', 'dense_budget', 'hybrid_budget', 'known_evidence')
$labels = @{ no_memory = 'Question only'; bm25_budget = 'BM25, 6k-token budget'; dense_budget = 'Dense, 6k-token budget'
    hybrid_budget = 'Hybrid, 6k-token budget'; known_evidence = 'Labelled evidence only' }
$summary = @(foreach ($condition in $order) {
    $group = @($rows | Where-Object condition -eq $condition)
    if (!$group.Count) { continue }
    $withEvidence = @($group | Where-Object { $_.evidence_in_context -ne $null })
    [pscustomobject]@{
        condition = $condition; questions = $group.Count
        judged_correct = @($group | Where-Object judged_correct).Count
        invalid_verdicts = @($group | Where-Object { $_.judge_verdict -notin @('yes', 'no') }).Count
        evidence_in_context = "$(@($withEvidence | Where-Object evidence_in_context).Count)/$($withEvidence.Count)"
        mean_input_tokens = [math]::Round(($group | Measure-Object input_tokens -Average).Average)
        mean_reader_seconds = [math]::Round(($group | Measure-Object reader_wall_ms -Average).Average / 1000, 1)
    }
})
$byType = [ordered]@{}
foreach ($type in ($rows | Select-Object -ExpandProperty question_type -Unique | Sort-Object)) {
    $byType[$type] = [ordered]@{}
    foreach ($condition in $order) {
        $group = @($rows | Where-Object { $_.question_type -eq $type -and $_.condition -eq $condition })
        if ($group.Count) { $byType[$type][$condition] = "$(@($group | Where-Object judged_correct).Count)/$($group.Count)" }
    }
}
$record = [ordered]@{
    schema_version = 1
    experiment = 'LongMemEval_S strict development questions answered by a benchmark-only local Qwen3-0.6B server'
    # Local paths are reduced to file names: the record is public.
    server = (Get-Content (Join-Path $RunDirectory 'server.json') -Raw) -replace '(?i)[A-Z]:(?:\\\\|/)(?:[^"\\\\/ ]+(?:\\\\|/))*', '' | ConvertFrom-Json
    reader_and_judge = 'qwen3-0.6b-q8_0-32k, thinking on, temperature 0 (greedy); output cap per case in output_token_limit'
    output_cap_note = 'Questions first run at 1024 output tokens whose thinking hit the cap were rerun at 4096; under greedy decoding a larger cap leaves replies that finished below 1024 unchanged'
    judge_adaptation = 'Official LongMemEval judge prompts (P04) answered by the same local Qwen instead of gpt-4o-2024-08-06'
    full_history = 'Not run: about 115k tokens exceeds the 32,768-token context'
    budget = '6000 estimated tokens (UTF-8 bytes / 4) of ranked user rounds, shown chronologically'
    summary = $summary; by_question_type = $byType; not_run = $failed; cases = $rows
}
[IO.File]::WriteAllText((Join-Path (Get-Location) $Output), ($record | ConvertTo-Json -Depth 6))
'| Condition | Judged correct | Evidence in context | Invalid verdicts | Mean input tokens | Mean reader s |'
'|---|---|---|---|---|---|'
foreach ($s in $summary) {
    $pct = if ($s.questions) { [math]::Round(100 * $s.judged_correct / $s.questions, 1) } else { 0 }
    "| $($labels[$s.condition]) | $($s.judged_correct)/$($s.questions) ($pct%) | $($s.evidence_in_context) | $($s.invalid_verdicts) | $($s.mean_input_tokens) | $($s.mean_reader_seconds) |"
}
