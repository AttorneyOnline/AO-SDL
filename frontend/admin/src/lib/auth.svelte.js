import { api, post, getAuthToken } from './api.js';

// Tokens live in sessionStorage, not localStorage:
// - sessionStorage is scoped to the browser tab and cleared on close,
//   so an XSS-stolen token stops working once the admin closes the tab.
// - localStorage would persist the token across browser restarts, giving
//   an attacker a much longer useful window.
// The trade-off is that closing the tab logs the admin out, which for an
// admin dashboard is an acceptable tax for XSS containment.
//
// We still proactively purge any legacy localStorage entries from earlier
// builds so an upgrade doesn't leave credentials behind on disk.
const LEGACY_KEYS = [
  'kagami_auth_token',
  'kagami_session_token',
  'kagami_username',
  'kagami_acl',
];
for (const k of LEGACY_KEYS) localStorage.removeItem(k);

/**
 * Auth state. Reactive via Svelte 5 $state.
 * Stores both the durable auth token and the ephemeral session token.
 */
export const auth = $state({
  /** @type {string} */ authToken: sessionStorage.getItem('kagami_auth_token') || '',
  /** @type {string} */ sessionToken: sessionStorage.getItem('kagami_session_token') || '',
  /** @type {string} */ username: sessionStorage.getItem('kagami_username') || '',
  /** @type {string} */ acl: sessionStorage.getItem('kagami_acl') || '',
  /** @type {boolean} */ loggedIn: !!sessionStorage.getItem('kagami_session_token'),
  /** @type {string} */ logoutReason: '',
});

/** Persist auth state to sessionStorage whenever it changes. */
function persist() {
  sessionStorage.setItem('kagami_auth_token', auth.authToken);
  sessionStorage.setItem('kagami_session_token', auth.sessionToken);
  sessionStorage.setItem('kagami_username', auth.username);
  sessionStorage.setItem('kagami_acl', auth.acl);
}

/**
 * Login: get auth token, then create an authenticated session.
 * @param {string} username
 * @param {string} password
 * @returns {Promise<{ok: boolean, error?: string}>}
 */
export async function login(username, password) {
  // Step 1: get durable auth token
  const loginRes = await post('/auth/login', { username, password });
  if (loginRes.status !== 201) {
    return { ok: false, error: loginRes.data?.reason || 'Login failed' };
  }

  const authToken = loginRes.data.token;

  // Step 2: create session with auth token
  const sessionRes = await post('/session', {
    client_name: 'kagami-admin',
    client_version: '1.0',
    hdid: 'admin-dashboard-' + crypto.randomUUID().slice(0, 8),
    auth: authToken,
  });

  if (sessionRes.status !== 201) {
    return { ok: false, error: sessionRes.data?.reason || 'Session creation failed' };
  }

  auth.authToken = authToken;
  auth.sessionToken = sessionRes.data.token;
  auth.username = loginRes.data.username;
  auth.acl = loginRes.data.acl;
  auth.loggedIn = true;
  persist();

  return { ok: true };
}

/**
 * Logout: revoke auth token, clear local state.
 */
export async function logout() {
  if (auth.authToken) {
    await post('/auth/logout', null, { token: auth.authToken });
  }
  auth.authToken = '';
  auth.sessionToken = '';
  auth.username = '';
  auth.acl = '';
  auth.loggedIn = false;
  // Remove rather than overwrite so no empty-string credential lingers
  // in the tab's storage bucket after logout.
  for (const k of ['kagami_auth_token', 'kagami_session_token', 'kagami_username', 'kagami_acl'])
    sessionStorage.removeItem(k);
}

/**
 * Renew the session token. Called periodically to prevent expiry.
 * If the session is expired, attempt to create a new one from the
 * auth token. If that fails too, log out.
 */
async function renewSession() {
  if (!auth.sessionToken) return;

  // Try renewing via PATCH /session
  const res = await api('PATCH', '/session', null);
  if (res.status === 200) return; // renewed

  // Session expired — try creating a new one from auth token
  if (auth.authToken) {
    const sessionRes = await post('/session', {
      client_name: 'kagami-admin',
      client_version: '1.0',
      hdid: 'admin-dashboard-' + crypto.randomUUID().slice(0, 8),
      auth: auth.authToken,
    });
    if (sessionRes.status === 201) {
      auth.sessionToken = sessionRes.data.token;
      persist();
      return;
    }
  }

  // Both failed — force logout with message
  auth.authToken = '';
  auth.sessionToken = '';
  auth.username = '';
  auth.acl = '';
  auth.loggedIn = false;
  auth.logoutReason = 'Session expired. The server may have restarted.';
  persist();
  window.location.hash = '#/login';
}

// Auto-renew session every 60 seconds while the dashboard is open
let _keepaliveInterval = null;
export function startKeepalive() {
  if (_keepaliveInterval) return;
  _keepaliveInterval = setInterval(renewSession, 60_000);
}
export function stopKeepalive() {
  if (_keepaliveInterval) {
    clearInterval(_keepaliveInterval);
    _keepaliveInterval = null;
  }
}
