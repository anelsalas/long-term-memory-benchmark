param([Parameter(Mandatory)][string]$RunDirectory, [switch]$Preview)
$ErrorActionPreference = 'Stop'

function Read-JsonFile($Path) { Get-Content -LiteralPath $Path -Raw | ConvertFrom-Json }
function Escape($Value) { [Net.WebUtility]::HtmlEncode([string]$Value).Replace("`n", '&#10;') }

# Read the reply before comparing with gold. Strip only the recurring timestamp
# and person identifier so their digits cannot be mistaken for a grade.
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

$baseline = Read-JsonFile 'reports/2026-11-01-qwen-controls.json'
$labels = [ordered]@{ no_memory='Question only'; full_history='Full history'; retrieved_evidence='Keyword retrieval'; known_evidence='Correction alone' }
$rows = @(foreach ($seed in 42..61) {
    $summaryFile = Join-Path $RunDirectory "result-$seed.json"
    if ($Preview -and !(Test-Path -LiteralPath $summaryFile)) { continue }
    $summary = Read-JsonFile $summaryFile
    if (!$summary.complete -or $summary.graded_cases -ne 4 -or $summary.provider_requests_made -ne 4) {
        throw "Incomplete seed $seed"
    }
    $config = Read-JsonFile (Join-Path $RunDirectory "seed-$seed.json")
    if ($config.output_token_limit -ne '500') { throw "Unexpected reply limit for seed $seed" }
    $dir = $summary.artifact_directory
    $records = (Read-JsonFile "$dir/results.json").records
    $plans = (Read-JsonFile "$dir/plan.json").cases
    $gold = (Read-JsonFile "$dir/evaluator-gold.json")[0].gold_answer
    $old = @($baseline.observations | Where-Object seed -eq $seed)
    foreach ($i in 0..3) {
        $plan = $plans[$i]
        $record = $records[$i]
        $previous = $old[$i]
        $reader = Read-JsonFile "$dir/case-$i.reader-result.json"
        if ($reader.attempts.Count -ne 1 -or $reader.attempts[0].output_token_limit -ne 500 -or
            $reader.response_metadata.model -ne $baseline.model -or $reader.attempts[0].reply -cne $record.answer) {
            throw "Model response mismatch for seed $seed"
        }
        if ($plan.prompt_sha256 -ne $previous.prompt_sha256 -or $plan.condition -ne $previous.condition -or
            $gold -ne $previous.gold -or ($plan.source_ids -join '|') -ne ($previous.source_ids -join '|') -or
            $record.case_id -ne "fresh:$($plan.condition)" -or $record.status -ne 'completed') {
            throw "Unmatched saved request or answer for seed $seed"
        }
        $stated = Read-StatedGrade $record.answer
        [pscustomobject]@{
            seed=$seed; condition=$plan.condition; expected_grade=$previous.gold; stated_grade=$stated
            correct_grade=($stated -eq [string]$previous.gold); exact_integer_match=$record.correct
            correction_supplied=$previous.support_in_context; prompt=$plan.prompt; source_ids=$plan.source_ids
            prompt_sha256=$plan.prompt_sha256; answer_at_32=$previous.answer; answer_at_500=$record.answer
            extends_earlier_reply=($record.answer.Length -gt $previous.answer.Length -and
                $record.answer.StartsWith($previous.answer, [StringComparison]::Ordinal))
        }
    }
})
if (!$rows.Count) { throw 'No completed cases' }
$cases = [int]($rows.Count / 4)
$counts = @(foreach ($method in $labels.Keys) {
    $group = @($rows | Where-Object condition -eq $method)
    [pscustomobject]@{
        condition=$method; cases=$group.Count; correct_grades=@($group | Where-Object correct_grade).Count
        exact_matches=@($group | Where-Object exact_integer_match).Count
        abstentions=@($group | Where-Object stated_grade -eq 'unknown').Count
        review=@($group | Where-Object stated_grade -eq 'review').Count
        with_correction=@($group | Where-Object correction_supplied).Count
    }
})
$results = [ordered]@{
    model=$baseline.model; requested_output_limit=500; baseline_output_limit=32; completed_cases=$cases
    continued_replies=@($rows | Where-Object extends_earlier_reply).Count
    score_readout='Retrospective reading of the stated grade, abstention or ambiguous reply. The original run uses exact integer match.'
    conditions=$counts; observations=$rows
}
if (!$Preview) {
    $results | ConvertTo-Json -Depth 12 | Set-Content (Join-Path $RunDirectory 'replay.json')
}
$tableRows = foreach ($g in $counts) {
    "<tr><th>$($labels[$g.condition])</th><td>$($g.correct_grades)/$cases</td><td>$($g.with_correction)/$cases</td><td>$($g.abstentions)</td><td>$($g.review)</td><td>$($g.exact_matches)/$cases</td></tr>"
}
$examples = foreach ($row in $rows) {
    $stated = Escape $row.stated_grade
    "<details class='card'><summary>Case $($row.seed) · $($labels[$row.condition]) · $stated</summary><p>Corrected grade: $(Escape $row.expected_grade)</p><p><strong>500-token limit</strong><br>$(Escape $row.answer_at_500)</p><p><strong>Earlier 32-token limit</strong><br>$(Escape $row.answer_at_32)</p><pre class='output'>$(Escape $row.prompt)</pre></details>"
}
$example = $rows | Where-Object { $_.seed -eq 42 -and $_.condition -eq 'full_history' }
$status = if ($Preview) { 'In progress' } else { 'Completed' }
$page = @"
<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1"><title>Qwen replay results</title><link rel="stylesheet" href="styles.css"></head><body>
<header class="app-header"><strong>Memory Benchmark</strong><a href="sonnet-first-case.html">Sonnet comparison</a><a href="/">Dashboard controls</a></header><main>
<div class="page-heading"><div><p class="eyebrow">$status · Local Qwen3-0.6B</p><h1>Does Qwen give the corrected grade?</h1><p>$cases of 20 histories completed. Four evidence conditions per history. Each reply can use up to 500 tokens.</p></div></div>
<article class="card"><h2>First case</h2><p>Lina's grade changed from 23 to 82. With the full history, Qwen replied:</p><blockquote>$(Escape $example.answer_at_500)</blockquote></article>
<article class="card table-scroll"><h2>Answers by memory condition</h2><table><thead><tr><th>Evidence</th><th>Correct grade stated</th><th>Correction supplied</th><th>Abstained</th><th>Mixed or unclear</th><th>Exact integer only</th></tr></thead><tbody>$($tableRows -join '')</tbody></table></article>
<p>Every prompt matches the earlier run byte for byte. $($results.continued_replies) of $($rows.Count) replies continued the exact text returned at the earlier limit. “Correct grade stated” reads the score in the answer, including complete sentences. “Exact integer only” applies the original benchmark rule. With no history, abstaining is appropriate. The 20 histories share one correction template.</p>
<p><a href="comparison.html">Earlier 32-token run</a></p><h2>Read each reply and its evidence</h2>$($examples -join [Environment]::NewLine)</main></body></html>
"@
$page | Set-Content web/qwen-replay.html
$counts | Format-Table
