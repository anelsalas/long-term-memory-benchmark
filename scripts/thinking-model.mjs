import {createHash} from 'node:crypto';
import {performance} from 'node:perf_hooks';

const conditions = ['no_memory', 'full_history', 'retrieved_evidence', 'known_evidence'];

// Same content-reading rule as the 500-token Qwen replay.
export function statedGrade(answer) {
  const text = answer.replaceAll('**', '').replace(/\b\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}Z\b/g, '')
    .replace(/\b(?:Lina|Robin)-\d+\b/g, 'student').trim();
  const numbers = text.match(/(?<![\w.-])\d{1,3}(?!\w|\.\d)/g) ?? [];
  const absent = /\b(?:unknown|no score|no recorded score|not recorded)\b/i.test(text);
  if (absent && numbers.length === 0) return 'unknown';
  if (!absent && numbers.length === 1 && Number(numbers[0]) <= 100) return numbers[0];
  return 'review';
}

// Public interface: answer(messages) resolves to {text, model}. The caller owns the implementation.
export async function runReaderValidation(reader, observations, seed, onResult = () => {}) {
  const cases = observations.filter(row => row.seed === seed);
  const seen = new Set(cases.map(row => row.condition));
  if (cases.length !== 4 || !conditions.every(name => seen.has(name))) throw Error('Incomplete control set');
  for (const row of cases) {
    if (typeof row.prompt !== 'string' || createHash('sha256').update(row.prompt).digest('hex') !== row.prompt_sha256)
      throw Error('Saved prompt fingerprint mismatch');
  }
  const results = [];
  for (const row of cases) {
    const start = performance.now();
    const reply = await reader.answer([{role: 'user', content: row.prompt}]);
    const wallMs = performance.now() - start;
    if (typeof reply.text !== 'string' || !reply.text.trim() || typeof reply.model !== 'string' || !reply.model)
      throw Error('Incomplete model response');
    const grade = statedGrade(reply.text);
    const result = {
      seed, condition: row.condition, prompt_sha256: row.prompt_sha256,
      expected_grade: row.expected_grade, correction_supplied: row.correction_supplied,
      model: reply.model, answer: reply.text, stated_grade: grade,
      correct_grade: grade === row.expected_grade, exact_integer_match: reply.text.trim() === row.expected_grade,
      wall_ms: Math.round(wallMs)
    };
    results.push(result);
    await onResult(result);
  }
  return results;
}
