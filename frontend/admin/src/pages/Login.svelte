<script>
  import { login } from '../lib/auth.svelte.js';
  import { KeyRound } from 'lucide-svelte';

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
  }
</script>

<div class="min-h-screen bg-(--color-surface-0) flex items-center justify-center px-4">
  <div class="w-full max-w-xs">
    <div class="bg-(--color-surface-1) border border-(--color-border) p-6">
      <div class="flex items-center gap-2 mb-1">
        <KeyRound size={18} strokeWidth={1.5} />
        <h1 class="text-lg font-semibold tracking-wide uppercase">Kagami</h1>
      </div>
      <p class="text-xs text-(--color-text-muted) mb-6">Server Administration</p>

      <form onsubmit={handleSubmit} class="space-y-4">
        <div>
          <label for="username" class="block text-xs font-medium text-(--color-text-secondary) mb-1">Username</label>
          <input
            id="username"
            type="text"
            bind:value={username}
            required
            autocomplete="username"
            class="w-full px-3 py-2 bg-(--color-surface-2) border border-(--color-border) text-sm text-(--color-text-primary)
                   placeholder:text-(--color-text-muted) focus:outline-none focus:border-(--color-border-active)"
            placeholder="root"
          />
        </div>

        <div>
          <label for="password" class="block text-xs font-medium text-(--color-text-secondary) mb-1">Password</label>
          <input
            id="password"
            type="password"
            bind:value={password}
            required
            autocomplete="current-password"
            class="w-full px-3 py-2 bg-(--color-surface-2) border border-(--color-border) text-sm text-(--color-text-primary)
                   placeholder:text-(--color-text-muted) focus:outline-none focus:border-(--color-border-active)"
          />
        </div>

        {#if error}
          <div class="text-xs text-red-500 bg-red-500/10 border border-red-500/20 px-3 py-2">{error}</div>
        {/if}

        <button
          type="submit"
          disabled={loading}
          class="w-full py-2 px-4 bg-(--color-accent) text-(--color-surface-0) text-sm font-medium
                 hover:opacity-80 disabled:opacity-30 transition-opacity"
        >
          {loading ? 'Signing in...' : 'Sign in'}
        </button>
      </form>
    </div>
  </div>
</div>
