param([string]$RunDirectory = 'runtime/four-controls')
$ErrorActionPreference = 'Stop'
function Escape([object]$value) { [System.Net.WebUtility]::HtmlEncode([string]$value) }
$rows = @()
foreach ($seed in 42..61) {
  $summary = Get-Content "$RunDirectory/result-$seed.json" -Raw | ConvertFrom-Json
  if (!$summary.complete -or $summary.provider_requests_made -ne 4) { throw "Incomplete seed $seed" }
  $folder = $summary.artifact_directory
  $plan = Get-Content "$folder/plan.json" -Raw | ConvertFrom-Json
  $records = (Get-Content "$folder/results.json" -Raw | ConvertFrom-Json).records
  $gold = (Get-Content "$folder/evaluator-gold.json" -Raw | ConvertFrom-Json)[0].gold_answer
  $support = @($plan.cases | Where-Object condition -eq 'known_evidence')[0].source_ids[0]
  foreach ($i in 0..3) {
    $detail = Get-Content "$folder/case-$i.result.json" -Raw | ConvertFrom-Json
    $rows += [pscustomobject]@{
      seed=$seed; condition=$plan.cases[$i].condition; gold=$gold
      answer=$records[$i].answer; correct=$records[$i].correct
      exact_abstention=($records[$i].answer.Trim() -eq 'UNKNOWN')
      support_in_context=($plan.cases[$i].source_ids -contains $support)
      reader_ms=$detail.reader_wall_ms; preparation_ms=$detail.retrieval_preparation_ms
      completion_tokens=$detail.response_metadata.usage.completion_tokens
      prompt_bytes=$plan.cases[$i].prompt_bytes
      source_ids=$plan.cases[$i].source_ids; prompt=$plan.cases[$i].prompt
      prompt_sha256=$plan.cases[$i].prompt_sha256
      configuration_hash=$detail.configuration_hash; resources=$detail.local_resources
    }
  }
}
$aggregates = foreach ($condition in @('no_memory','full_history','retrieved_evidence','known_evidence')) {
  $group = @($rows | Where-Object condition -eq $condition)
  $sorted = @($group.reader_ms | Sort-Object)
  [pscustomobject]@{
    condition=$condition; correct=@($group | Where-Object correct).Count; total=$group.Count
    exact_abstentions=@($group | Where-Object exact_abstention).Count
    supporting_evidence_cases=@($group | Where-Object support_in_context).Count
    median_reader_ms=($sorted[9]+$sorted[10])/2
    mean_prompt_bytes=($group.prompt_bytes | Measure-Object -Average).Average
    output_tokens=($group.completion_tokens | Measure-Object -Sum).Sum
    at_output_cap=@($group | Where-Object completion_tokens -eq 32).Count
    mean_runner_cpu_ms=($group.resources.cpu_delta_ms | Measure-Object -Average).Average
    max_runner_resident_bytes=($group.resources.resident_after_bytes | Measure-Object -Maximum).Maximum
  }
}
$executed = Get-Content "$RunDirectory/seed-42.json" -Raw | ConvertFrom-Json
$report = [ordered]@{ schema_version=1; model='qwen3-0.6b-q8_0'; seeds='42..61'; api_cost_usd=0
  source_revision=$executed.source_revision; output_limit=32; context_service_bytes=512
  grading='strict integer gold agreement; UNKNOWN abstention recorded separately'
  requests=$rows.Count; conditions=$aggregates; observations=$rows }
$report | ConvertTo-Json -Depth 16 | Set-Content reports/2026-11-01-qwen-controls.json
$summaryRows = ($aggregates | ForEach-Object {
  '<tr><th>'+ (Escape $_.condition) +'</th><td>'+ $_.correct +'/'+$_.total+'</td><td>'+$_.supporting_evidence_cases+'/20</td><td>'+[math]::Round($_.median_reader_ms,1)+'</td><td>'+[math]::Round($_.mean_prompt_bytes,1)+'</td><td>'+$_.output_tokens+'</td><td>'+$_.at_output_cap+'/20</td></tr>'
}) -join "`n"
$details = ($rows | ForEach-Object {
  '<details class="card"><summary>Seed '+$_.seed+' · '+(Escape $_.condition)+' · '+$(if ($_.correct) {'Correct'} else {'Gold mismatch'})+'</summary><p>Expected: <strong>'+(Escape $_.gold)+'</strong></p><p>Answer: '+(Escape $_.answer)+'</p><p>Reader: '+[math]::Round($_.reader_ms,1)+' ms · output tokens: '+$_.completion_tokens+'</p><pre class="output">'+(Escape $_.prompt)+'</pre></details>'
}) -join "`n"
$html = @"
<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1"><title>Qwen comparison results</title><link rel="stylesheet" href="styles.css"></head><body>
<header class="app-header"><div class="identity"><strong>Memory Benchmark · measured results</strong></div><a href="/">Dashboard controls</a></header>
<main><div class="page-heading"><div><p class="eyebrow">November 1, 2026 · native ARM64</p><h1>Qwen: four memory conditions</h1><p>20 correction histories · 80 real requests · API cost `$0</p></div></div>
<article class="card"><h2>What this checks</h2><p>The same Qwen3-0.6B model answers the same 20 questions using four evidence conditions. Known evidence supplies the actual correction record. Seeds 42–61 vary scores in one fixed story template; condition order rotates evenly.</p><p>Accuracy is strict agreement with the ledger's integer answer. For question-only requests, gold agreement is a guessing diagnostic; an UNKNOWN response is the requested abstention. Each request starts a new conversation.</p></article>
<article class="card table-scroll"><table><thead><tr><th>Evidence condition</th><th>Correct</th><th>Supporting evidence</th><th>Median reader ms</th><th>Mean prompt bytes</th><th>Output tokens</th><th>At 32-token cap</th></tr></thead><tbody>$summaryRows</tbody></table></article>
<article class="card"><h2>Next experiment</h2><p>Check Qwen's answer format and output allowance with known evidence first. Once it reliably answers those controls, compare memory methods on the same frozen cases. Preserve each prompt revision as a separate experiment.</p></article>
<article class="card"><h2>Interpretation and scope</h2><p>Inspect the recorded answers below to separate evidence selection from answer formatting and output limits. The prompt requests an integer or UNKNOWN. This prompt differs from the earlier two-request smoke, which requested only an integer.</p><p>Timing includes local model response and transport; the first request includes loading. Order is balanced; timings are descriptive. Input token usage is unavailable from this service. CPU and RAM samples in the downloadable repository report cover the runner process. Model-worker resources and energy remain separate measurements.</p><p>These compact development cases test correction handling. Broader task coverage and held-out evaluation follow before ranking memory systems.</p><a href="https://github.com/anelsalas/long-term-memory-benchmark/blob/main/reports/2026-11-01-qwen-controls.json">Machine-readable observations</a></article>
<h2>Every answer and its supplied evidence</h2>$details</main></body></html>
"@
$html | Set-Content web/comparison.html
$aggregates | Format-Table

