# Compares readers on the same LongMemEval_S questions, conditions and retrieved
# evidence. Each -Reader is "label|run directory|verdicts|cross verdicts":
#   verdicts:       'in-run' to use the judge ltm-compare called, or an ltm-rejudge
#                   results file (for answers first graded by another judge);
#   cross verdicts: optional ltm-rejudge results from a second judge, for agreement.
# Writes a record without dialogue text: verdicts, evidence presence, token counts.
param(
    [Parameter(Mandatory)] [string[]]$Reader,
    [Parameter(Mandatory)] [string]$Judge,       # the judge behind the primary verdicts
    [string]$CrossJudge = '',
    [string]$Singletons = 'runtime/retrieval-baselines/singletons/singletons.json',
    [Parameter(Mandatory)] [string]$Output
)
$ErrorActionPreference = 'Stop'
$types = @{}
foreach ($q in (Get-Content $Singletons -Raw | ConvertFrom-Json).questions) { $types[$q.question_id] = $q.question_type }
function strict_verdict([string]$reply) { $t = $reply.Trim().ToLower(); if ($t -in @('yes', 'no')) { $t } else { 'invalid' } }
function load_verdicts([string]$path) {
    $map = @{}
    if ($path -and $path -ne 'in-run') { foreach ($v in (Get-Content $path -Raw -Encoding UTF8 | ConvertFrom-Json).verdicts) { $map[$v.key] = $v.verdict } }
    return $map
}

$rows = @()
foreach ($spec in $Reader) {
    $label, $runDir, $primary, $cross = $spec.Split('|')
    $primaryMap = load_verdicts $primary
    $crossMap = load_verdicts $cross
    $cases = @{}; $known = @{}
    foreach ($file in Get-ChildItem $runDir -Filter '*.result.json' | Sort-Object Name) {
        $id = ($file.Name -replace '\.result\.json$', '').Split('.')[0]
        $dir = (Get-Content $file.FullName -Raw | ConvertFrom-Json).artifact_directory
        $plan = Get-Content (Join-Path $dir 'plan.json') -Raw | ConvertFrom-Json
        for ($i = 0; $i -lt $plan.cases.Count; $i++) {
            $case = $plan.cases[$i]
            if ($case.condition -eq 'known_evidence') { $known[$id] = @($case.source_ids) }
            $cases["$id|$($case.condition)"] = @{ id = $id; case = $case; result = Get-Content (Join-Path $dir "case-$i.result.json") -Raw -Encoding UTF8 | ConvertFrom-Json }
        }
    }
    foreach ($key in $cases.Keys | Sort-Object) {
        $c = $cases[$key]; $id = $c.id; $case = $c.case; $result = $c.result
        $inRun = if ($result.judge -and $result.judge.attempts[-1].reply) { strict_verdict $result.judge.attempts[-1].reply } else { $null }
        $answered = [bool]$result.judge  # an answer within the byte cap reaches the judge
        $verdict = if (!$answered) { 'unjudged' } elseif ($primary -eq 'in-run') { if ($inRun) { $inRun } else { 'failed' } } elseif ($primaryMap.ContainsKey($key)) { $primaryMap[$key] } else { 'missing' }
        $evidence = if ($known[$id]) { @($case.source_ids | Where-Object { $known[$id] -contains $_ }).Count -gt 0 } else { $null }
        $rows += [pscustomobject]@{
            reader = $label; question_id = $id; question_type = $types[$id]; condition = $case.condition
            verdict = $verdict; correct = $verdict -eq 'yes'
            original_verdict = if ($primary -ne 'in-run') { $inRun } else { $null }
            cross_verdict = if ($crossMap.ContainsKey($key)) { $crossMap[$key] } else { $null }
            evidence_in_context = $evidence; sources_sha256 = (& { $s = [Security.Cryptography.SHA256]::Create(); -join ($s.ComputeHash([Text.Encoding]::UTF8.GetBytes((@($case.source_ids) -join "`n"))) | ForEach-Object { $_.ToString('x2') }) })
            input_tokens = $result.attempts[-1].usage.input_tokens; output_tokens = $result.attempts[-1].usage.output_tokens
            prompt_bytes = $case.prompt_bytes; reader_wall_ms = [math]::Round($result.reader_wall_ms)
        }
    }
}

$order = @('no_memory', 'bm25_budget', 'dense_budget', 'hybrid_budget', 'known_evidence')
$labels = @{ no_memory = 'Question only'; bm25_budget = 'BM25, 6k budget'; dense_budget = 'Dense, 6k budget'
    hybrid_budget = 'Hybrid, 6k budget'; known_evidence = 'Labelled evidence only' }
$readers = @($Reader | ForEach-Object { $_.Split('|')[0] })
$summary = @(foreach ($label in $readers) { foreach ($condition in $order) {
    $group = @($rows | Where-Object { $_.reader -eq $label -and $_.condition -eq $condition })
    if (!$group.Count) { continue }
    $withEvidence = @($group | Where-Object { $_.evidence_in_context -ne $null })
    [pscustomobject]@{
        reader = $label; condition = $condition; questions = $group.Count
        judged_correct = @($group | Where-Object correct).Count
        no_answer = @($group | Where-Object verdict -eq 'unjudged').Count
        invalid_or_failed = @($group | Where-Object { $_.verdict -in @('invalid', 'failed', 'missing') }).Count
        evidence_in_context = "$(@($withEvidence | Where-Object evidence_in_context).Count)/$($withEvidence.Count)"
        mean_input_tokens = [math]::Round(($group | Measure-Object input_tokens -Average).Average)
        mean_reader_seconds = [math]::Round(($group | Measure-Object reader_wall_ms -Average).Average / 1000, 1)
    }
} })
$byType = [ordered]@{}
foreach ($type in ($rows | Select-Object -ExpandProperty question_type -Unique | Sort-Object)) {
    $byType[$type] = [ordered]@{}
    foreach ($label in $readers) { foreach ($condition in $order) {
        $group = @($rows | Where-Object { $_.reader -eq $label -and $_.question_type -eq $type -and $_.condition -eq $condition })
        if ($group.Count) { $byType[$type]["$label|$condition"] = "$(@($group | Where-Object correct).Count)/$($group.Count)" }
    } }
}
# The same packed evidence for every reader: compare source lists per question and condition.
$evidenceCheck = @(foreach ($g in $rows | Group-Object { "$($_.question_id)|$($_.condition)" }) {
    [pscustomobject]@{ key = $g.Name; readers = $g.Count; identical = @($g.Group.sources_sha256 | Select-Object -Unique).Count -eq 1 }
})
function agreement($pairs) {
    $pairs = @($pairs | Where-Object { $_[0] -in @('yes', 'no') -and $_[1] -in @('yes', 'no') })
    [ordered]@{ compared = $pairs.Count; agree = @($pairs | Where-Object { $_[0] -eq $_[1] }).Count }
}
$agreements = [ordered]@{}
foreach ($label in $readers) {
    $mine = @($rows | Where-Object reader -eq $label)
    $a = [ordered]@{}
    if (@($mine | Where-Object original_verdict).Count) { $a['original_judge_vs_primary'] = agreement @($mine | ForEach-Object { , @($_.original_verdict, $_.verdict) }) }
    if (@($mine | Where-Object cross_verdict).Count) { $a['primary_vs_cross'] = agreement @($mine | ForEach-Object { , @($_.verdict, $_.cross_verdict) }) }
    $agreements[$label] = $a
}
$record = [ordered]@{
    schema_version = 1
    experiment = 'LongMemEval_S strict development questions: the same retrieved evidence read by different models'
    judge = $Judge; cross_judge = $CrossJudge
    readers = $Reader | ForEach-Object { $_.Split('|')[0] }
    budget = '6000 estimated tokens (UTF-8 bytes / 4) of ranked user rounds, shown chronologically'
    identical_evidence = "$(@($evidenceCheck | Where-Object { $_.identical -and $_.readers -eq $readers.Count }).Count)/$($evidenceCheck.Count) question-condition pairs"
    summary = $summary; by_question_type = $byType; judge_agreement = $agreements; cases = $rows
}
[IO.File]::WriteAllText((Join-Path (Get-Location) $Output), ($record | ConvertTo-Json -Depth 6), (New-Object Text.UTF8Encoding $false))
"Identical evidence across readers: $($record.identical_evidence)"
'| Condition | ' + ($readers -join ' | ') + ' |'
'|---|' + (($readers | ForEach-Object { '---' }) -join '|') + '|'
foreach ($condition in $order) {
    $cells = foreach ($label in $readers) {
        $s = $summary | Where-Object { $_.reader -eq $label -and $_.condition -eq $condition }
        if ($s) { "$($s.judged_correct)/$($s.questions) ($([math]::Round(100 * $s.judged_correct / $s.questions, 1))%)" } else { '' }
    }
    "| $($labels[$condition]) | $($cells -join ' | ') |"
}
$agreements | ConvertTo-Json -Depth 4
