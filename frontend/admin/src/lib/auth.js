import { post, getAuthToken } from './api.js';

/**
 * Auth state. Reactive via Svelte 5 $state.
 * Stores both the durable auth token and the ephemeral session token.
 */
export const auth = $state({
  /** @type {string} */ authToken: localStorage.getItem('kagami_auth_token') || '',
  /** @type {string} */ sessionToken: localStorage.getItem('kagami_session_token') || '',
  /** @type {string} */ username: localStorage.getItem('kagami_username') || '',
  /** @type {string} */ acl: localStorage.getItem('kagami_acl') || '',
  /** @type {boolean} */ loggedIn: !!localStorage.getItem('kagami_session_token'),
});

/** Persist auth state to localStorage whenever it changes. */
function persist() {
  localStorage.setItem('kagami_auth_token', auth.authToken);
  localStorage.setItem('kagami_session_token', auth.sessionToken);
  localStorage.setItem('kagami_username', auth.username);
  localStorage.setItem('kagami_acl', auth.acl);
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
  persist();
}

/**
 * Check if the current session is still valid.
 * @returns {Promise<boolean>}
 */
export async function checkSession() {
  if (!auth.sessionToken) return false;
  const res = await post('/session', null, { token: auth.sessionToken });
  // PATCH /session (renew) would be better, but GET /server works as a health check
  return res.status !== 401;
}
