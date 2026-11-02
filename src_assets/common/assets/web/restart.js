// A restart can close its own HTTP connection. Keep the current page until
// the host responds again, rather than navigating into a connection error.
export function normalizeConfigUrl(href) {
  return href.endsWith('#') ? href.slice(0, -1) : href;
}

export async function restartAndReconnect({
  request = fetch,
  sleep = ms => new Promise(resolve => setTimeout(resolve, ms)),
  href = window.location.href,
  navigate = url => window.location.replace(url),
} = {}) {
  try {
    const response = await request('./api/restart', {
      method: 'POST', headers: {'Content-Type': 'application/json'},
    });
    if (!response.ok) throw new Error(`Restart rejected (${response.status})`);
  } catch (error) {
    // Only network failures are expected while the host shuts down.
    if (!(error instanceof TypeError)) throw error;
  }
  for (let attempt = 0; attempt < 30; attempt++) {
    await sleep(1000);
    const controller = new AbortController();
    const timeout = setTimeout(() => controller.abort(), 2000);
    try {
      const response = await request('./api/config', {
        cache: 'no-store', signal: controller.signal,
      });
      if (response.ok || response.status === 401) {
        navigate(normalizeConfigUrl(href));
        return;
      }
    } catch {
      // Connection refused/aborted while restarting: keep waiting.
    } finally {
      clearTimeout(timeout);
    }
  }
  throw new Error('ApolloReady did not become available after restarting');
}
