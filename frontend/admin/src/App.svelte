<script>
  import { auth, startKeepalive, stopKeepalive } from './lib/auth.svelte.js';
  import Nav from './components/Nav.svelte';
  import Login from './pages/Login.svelte';
  import Dashboard from './pages/Dashboard.svelte';
  import Config from './pages/Config.svelte';
  import Sessions from './pages/Sessions.svelte';
  import Traffic from './pages/Traffic.svelte';
  import Bans from './pages/Bans.svelte';
  import Moderation from './pages/Moderation.svelte';
  import Areas from './pages/Areas.svelte';
  import Users from './pages/Users.svelte';
  import Firewall from './pages/Firewall.svelte';
  import Content from './pages/Content.svelte';

  let hash = $state(window.location.hash || '#/login');

  function onHashChange() {
    hash = window.location.hash || '#/login';
  }

  $effect(() => {
    window.addEventListener('hashchange', onHashChange);
    return () => window.removeEventListener('hashchange', onHashChange);
  });

  // Start/stop session keepalive based on login state
  $effect(() => {
    if (auth.loggedIn) startKeepalive();
    else stopKeepalive();
    return () => stopKeepalive();
  });

  // Redirect to login if not authenticated
  let page = $derived.by(() => {
    if (!auth.loggedIn && hash !== '#/login') {
      window.location.hash = '#/login';
      return 'login';
    }
    if (auth.loggedIn && hash === '#/login') {
      window.location.hash = '#/dashboard';
      return 'dashboard';
    }
    return hash.slice(2) || 'login'; // strip '#/'
  });
</script>

{#if !auth.loggedIn}
  <Login />
{:else}
  <div class="flex h-screen bg-(--color-surface-0) text-(--color-text-primary)">
    <Nav currentPage={page} />
    <main class="flex-1 overflow-auto p-4 md:p-6 lg:p-8">
      {#if page === 'dashboard'}
        <Dashboard />
      {:else if page === 'config'}
        <Config />
      {:else if page === 'sessions'}
        <Sessions />
      {:else if page === 'traffic'}
        <Traffic />
      {:else if page === 'bans'}
        <Bans />
      {:else if page === 'moderation'}
        <Moderation />
      {:else if page === 'areas'}
        <Areas />
      {:else if page === 'users'}
        <Users />
      {:else if page === 'firewall'}
        <Firewall />
      {:else if page === 'content'}
        <Content />
      {:else}
        <div class="text-center text-gray-500 mt-20">
          <h2 class="text-2xl font-semibold mb-2">Coming Soon</h2>
          <p>The <code class="text-gray-400">{page}</code> page is not yet implemented.</p>
        </div>
      {/if}
    </main>
  </div>
{/if}
