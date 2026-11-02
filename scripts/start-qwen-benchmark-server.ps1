# Starts a benchmark-only Qwen3 server on loopback with the model's full native
# context (32,768 tokens). This is separate from the Dumas voice service on 8766,
# whose 512-byte prompts and 1024-token context are tuned for short spoken replies.
# Use a clang build of llama.cpp on Windows ARM64: the MSVC build read prompts about 3x slower
# here (87 vs 290 tokens/s for Qwen3-0.6B).
param(
    [Parameter(Mandatory)] [string]$Server,  # llama-server.exe
    [Parameter(Mandatory)] [string]$Model,   # Qwen3-0.6B-Q8_0.gguf
    [int]$Port = 8772,
    [int]$Context = 32768,
    [int]$Threads = 8,
    [int]$BatchThreads = 16,  # prompt reading scales further than generation
    [string]$Alias = 'qwen3-0.6b-q8_0-32k'
)
$ErrorActionPreference = 'Stop'
$arguments = @(
    '--model', "`"$Model`"", '--host', '127.0.0.1', '--port', $Port, '--alias', $Alias,
    '--ctx-size', $Context, '--parallel', '1', '--threads', $Threads, '--threads-batch', $BatchThreads,
    '--flash-attn', 'on', '--cache-type-k', 'q8_0', '--cache-type-v', 'q8_0', '--jinja'
)
$process = Start-Process -FilePath $Server -ArgumentList $arguments -PassThru -WindowStyle Hidden
for ($i = 0; $i -lt 120; $i++) {
    try {
        $health = Invoke-RestMethod "http://127.0.0.1:$Port/health" -TimeoutSec 2
        if ($health.status -eq 'ok') { Write-Output "Qwen benchmark server ready: pid $($process.Id), port $Port, context $Context"; exit 0 }
    } catch {}
    if ($process.HasExited) { throw "llama-server exited with code $($process.ExitCode)" }
    Start-Sleep -Milliseconds 500
}
throw 'llama-server did not become ready within 60 s'
