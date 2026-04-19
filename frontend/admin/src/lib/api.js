/** @type {string} */
const BASE = '/aonx/v1';

/** Get the session token from sessionStorage. */
export function getSessionToken() {
  return sessionStorage.getItem('kagami_session_token') || '';
}

/** Get the auth token from sessionStorage. */
export function getAuthToken() {
  return sessionStorage.getItem('kagami_auth_token') || '';
}

/**
 * Make an authenticated API call.
 * @param {string} method
 * @param {string} path - relative to /aonx/v1
 * @param {object} [body]
 * @param {object} [opts] - { token: override token, raw: return Response }
 * @returns {Promise<{status: number, data: any}>}
 */
export async function api(method, path, body = null, opts = {}) {
  const token = opts.token || getSessionToken();
  const headers = { 'Content-Type': 'application/json' };
  if (token) headers['Authorization'] = `Bearer ${token}`;

  const res = await fetch(BASE + path, {
    method,
    headers,
    body: body ? JSON.stringify(body) : null,
  });

  if (opts.raw) return res;

  let data = null;
  const text = await res.text();
  if (text) {
    try {
      data = JSON.parse(text);
    } catch {
      data = text;
    }
  }
  return { status: res.status, data };
}

// Convenience wrappers
export const get = (path, opts) => api('GET', path, null, opts);
export const post = (path, body, opts) => api('POST', path, body, opts);
export const patch = (path, body, opts) => api('PATCH', path, body, opts);
export const del = (path, opts) => api('DELETE', path, null, opts);

/**
 * Fetch and parse Prometheus /metrics text into a Map<string, number>.
 * Only extracts gauge/counter values (ignores histograms, help, type lines).
 */
export async function fetchMetrics() {
  const res = await fetch('/metrics');
  if (!res.ok) return new Map();
  const text = await res.text();
  /** @type {Map<string, number>} */
  const metrics = new Map();
  for (const line of text.split('\n')) {
    if (!line || line.startsWith('#')) continue;
    // Format: metric_name{labels} value [timestamp]
    // or:    metric_name value [timestamp]
    const match = line.match(/^([a-zA-Z_:][a-zA-Z0-9_:]*(?:\{[^}]*\})?)\s+([\d.eE+-]+(?:nan|inf)?)/i);
    if (match) metrics.set(match[1], parseFloat(match[2]));
  }
  return metrics;
}
