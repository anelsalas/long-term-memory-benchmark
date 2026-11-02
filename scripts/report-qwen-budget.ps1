# Summarizes runtime/qwen-budget into a public JSON record and a Markdown table.
# The grade reading is the same rule as scripts/report-qwen-replay.ps1.
param(
    [string]$RunDirectory = 'runtime/qwen-budget',
    [string]$Output = 'reports/2026-11-01-qwen-budget.json'
)
$ErrorActionPreference = 'Stop'

function Read-StatedGrade([string]$Reply) {
    $text = $Reply.Replace('**', '').Trim() -replace '\b\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}Z\b',''
    $text = $text -replace '\b(?:Lina|Robin)-\d+\b','student'
    $values = @([regex]::Matches($text, '(?<![\w.-])\d{1,3}(?!\w|\.\d)'))
    $absent = $text -match '\b(?:unknown|no score|no recorded score|not recorded)\b'
    if ($absent -and $values.Count -eq 0) { return 'unknown' }
    if (!$absent -and $values.Count -eq 1 -and [int]$values[0].Value -le 100) {
        return $values[0].Value
    }
    return 'review'
}

$rows = @(foreach ($file in Get-ChildItem $RunDirectory -Filter 'result-*.json' | Sort-Object Name) {
    $summary = Get-Content $file.FullName -Raw | ConvertFrom-Json
    if (!$summary.complete) { throw "Incomplete run: $($file.Name)" }
    $dir = $summary.artifact_directory
    $plan = Get-Content (Join-Path $dir 'plan.json') -Raw | ConvertFrom-Json
    $gold = (Get-Content (Join-Path $dir 'evaluator-gold.json') -Raw | ConvertFrom-Json)[0].gold_answer
    $known = ($plan.cases | Where-Object condition -eq 'known_evidence').source_ids
    $seed = [int]($file.BaseName -replace 'result-', '')
    for ($i = 0; $i -lt $plan.cases.Count; $i++) {
        $result = Get-Content (Join-Path $dir "case-$i.result.json") -Raw | ConvertFrom-Json
        $reply = [string]$result.attempts[-1].reply
        $stated = Read-StatedGrade $reply
        [pscustomobject]@{
            seed = $seed; condition = $plan.cases[$i].condition; expected_grade = $gold
            stated_grade = $stated; correct_grade = ($stated -eq [string]$gold)
            exact_integer_match = [bool]$result.fresh_exact_answer_match
            correction_in_context = [bool](@($plan.cases[$i].source_ids) | Where-Object { $known -contains $_ })
            prompt_bytes = $plan.cases[$i].prompt_bytes; reader_wall_ms = $result.reader_wall_ms
            prompt = $plan.cases[$i].prompt; answer = $reply
        }
    }
})
$order = @('no_memory', 'retrieved_evidence', 'bm25_budget', 'known_evidence')
$labels = @{ no_memory = 'Question only'; retrieved_evidence = 'Keyword top-1'; bm25_budget = 'BM25, 50-token budget'; known_evidence = 'Correction alone' }
$summary = @(foreach ($condition in $order) {
    $group = @($rows | Where-Object condition -eq $condition)
    [pscustomobject]@{
        condition = $condition; cases = $group.Count
        correct_grades = @($group | Where-Object correct_grade).Count
        correction_in_context = @($group | Where-Object correction_in_context).Count
        abstentions = @($group | Where-Object stated_grade -eq 'unknown').Count
        review = @($group | Where-Object stated_grade -eq 'review').Count
        exact_matches = @($group | Where-Object exact_integer_match).Count
        mean_prompt_bytes = [math]::Round(($group | Measure-Object prompt_bytes -Average).Average)
    }
})
$record = [ordered]@{
    schema_version = 1
    experiment = 'Fresh correction histories with 6 distractors; local Qwen3-0.6B; four conditions'
    reader = 'qwen3-0.6b-q8_0 via the local resident service (127.0.0.1:8766), 500-token reply limit'
    full_history = 'Not run: with 6 distractors the full history exceeds the local 512-byte prompt cap'
    budget = '50 estimated tokens (UTF-8 bytes / 4) of BM25-ranked user rounds, shown chronologically'
    score_readout = 'Stated grade read with the rule from scripts/report-qwen-replay.ps1; exact integer match is the original strict scorer'
    summary = $summary
    cases = $rows
}
[IO.File]::WriteAllText((Join-Path (Get-Location) $Output), ($record | ConvertTo-Json -Depth 6))
'| Condition | Correct grade | Correction in context | Said unknown | Needs review | Strict integer match | Mean prompt bytes |'
'|---|---|---|---|---|---|---|'
foreach ($s in $summary) {
    "| $($labels[$s.condition]) | $($s.correct_grades)/$($s.cases) | $($s.correction_in_context)/$($s.cases) | $($s.abstentions) | $($s.review) | $($s.exact_matches)/$($s.cases) | $($s.mean_prompt_bytes) |"
}
