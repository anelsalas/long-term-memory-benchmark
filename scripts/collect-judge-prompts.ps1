# Collects the official judge prompts that ltm-compare saved for each answer in a
# run directory (one run per question or per question and condition), keyed
# "<question_id>|<condition>", for ltm-rejudge. Answers that never reached the
# judge (no reply) are listed separately; they count as not correct.
param(
    [Parameter(Mandatory)] [string]$RunDirectory,
    [Parameter(Mandatory)] [string]$Output
)
$ErrorActionPreference = 'Stop'
$items = @(); $unjudged = @()
foreach ($file in Get-ChildItem $RunDirectory -Filter '*.result.json' | Sort-Object Name) {
    $id = ($file.Name -replace '\.result\.json$', '').Split('.')[0]
    $dir = (Get-Content $file.FullName -Raw | ConvertFrom-Json).artifact_directory
    $plan = Get-Content (Join-Path $dir 'plan.json') -Raw | ConvertFrom-Json
    for ($i = 0; $i -lt $plan.cases.Count; $i++) {
        $key = "$id|$($plan.cases[$i].condition)"
        $intent = Join-Path $dir "case-$i.judge-intent.json"
        if (Test-Path $intent) { $items += [ordered]@{ key = $key; prompt = (Get-Content $intent -Raw -Encoding UTF8 | ConvertFrom-Json).prompt } }
        else { $unjudged += $key }
    }
}
$record = [ordered]@{ run_directory = (Split-Path $RunDirectory -Leaf); items = $items; unjudged = $unjudged }
[IO.File]::WriteAllText((Join-Path (Get-Location) $Output), ($record | ConvertTo-Json -Depth 4), (New-Object Text.UTF8Encoding $false))
"$($items.Count) judge prompts, $($unjudged.Count) answers never judged"
