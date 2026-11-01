import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import test from 'node:test';
import {runReaderValidation, statedGrade} from './thinking-model.mjs';
const {observations} = JSON.parse(await readFile(new URL('../reports/2026-11-01-qwen-500-replay.json', import.meta.url)));
test('the same 80 archived answers receive the Qwen content grades', () => {
  for (const row of observations) assert.equal(statedGrade(row.answer_at_500), row.stated_grade);
});
test('each reader receives only one original user prompt, with evaluator fields isolated', async () => {
  const cases = observations.filter(row => row.seed === 42);
  let calls = 0;
  const reader = {async answer(messages) {
    assert.deepEqual(messages, [{role:'user', content:cases[calls++].prompt}]);
    return {text:'82', model:'fixture'};
  }};
  const results = await runReaderValidation(reader, cases, 42);
  assert.equal(calls, 4);
  assert.ok(results.every(row => row.correct_grade && row.exact_integer_match));
  const altered = cases.map((row,i) => i === 3 ? {...row, prompt:'different'} : row);
  await assert.rejects(runReaderValidation(reader, altered, 42), /fingerprint/);
  assert.equal(calls, 4);
});
