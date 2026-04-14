<script>
  import { login } from '../lib/auth.svelte.js';

  let username = $state('');
  let password = $state('');
  let error = $state('');
  let loading = $state(false);

  async function handleSubmit(e) {
    e.preventDefault();
    error = '';
    loading = true;
    const result = await login(username, password);
    loading = false;
    if (!result.ok) {
      error = result.error;
    }
    // App.svelte handles redirect on auth.loggedIn change
  }
</script>

<div class="min-h-screen bg-gray-950 flex items-center justify-center px-4">
  <div class="w-full max-w-sm">
    <div class="bg-gray-900 rounded-xl border border-gray-800 p-6 shadow-2xl">
      <h1 class="text-xl font-bold text-white mb-1">Kagami</h1>
      <p class="text-sm text-gray-500 mb-6">Server Administration</p>

      <form onsubmit={handleSubmit} class="space-y-4">
        <div>
          <label for="username" class="block text-xs font-medium text-gray-400 mb-1">Username</label>
          <input
            id="username"
            type="text"
            bind:value={username}
            required
            autocomplete="username"
            class="w-full px-3 py-2 bg-gray-800 border border-gray-700 rounded-lg text-sm text-white
                   placeholder-gray-500 focus:outline-none focus:border-blue-500 focus:ring-1 focus:ring-blue-500"
            placeholder="root"
          />
        </div>

        <div>
          <label for="password" class="block text-xs font-medium text-gray-400 mb-1">Password</label>
          <input
            id="password"
            type="password"
            bind:value={password}
            required
            autocomplete="current-password"
            class="w-full px-3 py-2 bg-gray-800 border border-gray-700 rounded-lg text-sm text-white
                   placeholder-gray-500 focus:outline-none focus:border-blue-500 focus:ring-1 focus:ring-blue-500"
          />
        </div>

        {#if error}
          <div class="text-sm text-red-400 bg-red-400/10 rounded-lg px-3 py-2">{error}</div>
        {/if}

        <button
          type="submit"
          disabled={loading}
          class="w-full py-2 px-4 bg-blue-600 hover:bg-blue-500 disabled:bg-gray-700 disabled:text-gray-500
                 text-white text-sm font-medium rounded-lg transition-colors"
        >
          {loading ? 'Signing in...' : 'Sign in'}
        </button>
      </form>
    </div>
  </div>
</div>
