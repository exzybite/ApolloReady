import assert from 'node:assert/strict';
import fs from 'node:fs/promises';

const source = await fs.readFile(new URL('../src_assets/common/assets/web/restart.js', import.meta.url), 'utf8');
const {normalizeConfigUrl, restartAndReconnect} = await import(`data:text/javascript;base64,${Buffer.from(source).toString('base64')}`);
assert.equal(normalizeConfigUrl('https://localhost:47990/config#'), 'https://localhost:47990/config');
assert.equal(normalizeConfigUrl('https://localhost:47990/config#av'), 'https://localhost:47990/config#av');
const run = async responses => {
  const visited = [];
  const requests = [];
  let sleeps = 0;
  await restartAndReconnect({
    href: 'https://localhost:47990/config#',
    request: async (url, options) => {
      requests.push({url, options});
      const result = responses.shift();
      if (result instanceof Error) throw result;
      return result;
    },
    sleep: async () => { sleeps++; },
    navigate: url => visited.push(url),
  });
  return {visited, requests, sleeps};
};
const recovery = await run([new TypeError('connection closed'), new TypeError('connection refused'), {ok:false,status:503}, {ok:true,status:200}]);
assert.equal(recovery.sleeps, 3);
assert.deepEqual(recovery.visited, ['https://localhost:47990/config']);
assert.equal(recovery.requests[1].options.cache, 'no-store');
assert.equal(recovery.requests[0].options.method, 'POST');
assert.equal(recovery.requests[0].options.headers['Content-Type'], 'application/json');
await assert.rejects(run([{ok:false,status:403}]), /Restart rejected/);
const expiredLogin = await run([{ok:true,status:200}, {ok:false,status:401}]);
assert.equal(expiredLogin.visited.length, 1);
await assert.rejects(run([{ok:true,status:200}, ...Array.from({length:30}, () => new TypeError('offline'))]), /did not become available/);
console.log('Restart reconnection tests passed');
