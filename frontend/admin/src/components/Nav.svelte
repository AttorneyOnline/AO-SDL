<script>
  import { auth, logout } from '../lib/auth.js';

  let { currentPage = 'dashboard' } = $props();
  let mobileOpen = $state(false);

  const links = [
    { page: 'dashboard', label: 'Dashboard', icon: '\u{1F3E0}' },
    { page: 'sessions',  label: 'Sessions',  icon: '\u{1F465}' },
    { page: 'traffic',   label: 'Traffic',   icon: '\u{1F4AC}' },
    { page: 'config',    label: 'Config',    icon: '\u{2699}' },
    { page: 'bans',      label: 'Bans',      icon: '\u{1F6AB}' },
    { page: 'moderation',label: 'Moderation', icon: '\u{1F6E1}' },
    { page: 'areas',     label: 'Areas',     icon: '\u{1F5FA}' },
    { page: 'users',     label: 'Users',     icon: '\u{1F511}' },
    { page: 'firewall',  label: 'Firewall',  icon: '\u{1F525}' },
    { page: 'content',   label: 'Content',   icon: '\u{1F4C1}' },
  ];

  function navigate(page) {
    window.location.hash = '#/' + page;
    mobileOpen = false;
  }

  async function handleLogout() {
    await logout();
    window.location.hash = '#/login';
  }
</script>

<!-- Mobile hamburger -->
<button
  class="md:hidden fixed top-3 left-3 z-50 p-2 bg-gray-800 rounded-lg border border-gray-700"
  onclick={() => mobileOpen = !mobileOpen}
  aria-label="Toggle navigation"
>
  <svg class="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
    {#if mobileOpen}
      <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M6 18L18 6M6 6l12 12" />
    {:else}
      <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M4 6h16M4 12h16M4 18h16" />
    {/if}
  </svg>
</button>

<!-- Sidebar -->
<!-- svelte-ignore a11y_click_events_have_key_events -->
<!-- svelte-ignore a11y_no_static_element_interactions -->
<aside
  class="fixed md:static inset-y-0 left-0 z-40 w-56 bg-gray-900 border-r border-gray-800
         flex flex-col transition-transform duration-200 ease-in-out
         {mobileOpen ? 'translate-x-0' : '-translate-x-full'} md:translate-x-0"
>
  <!-- Header -->
  <div class="p-4 border-b border-gray-800">
    <h1 class="text-lg font-bold tracking-tight">Kagami</h1>
    <p class="text-xs text-gray-500">{auth.username} &middot; {auth.acl}</p>
  </div>

  <!-- Nav links -->
  <nav class="flex-1 overflow-y-auto py-2">
    {#each links as { page, label, icon }}
      <button
        class="w-full flex items-center gap-3 px-4 py-2 text-sm transition-colors
               {currentPage === page ? 'bg-gray-800 text-white' : 'text-gray-400 hover:bg-gray-800/50 hover:text-gray-200'}"
        onclick={() => navigate(page)}
      >
        <span class="text-base">{icon}</span>
        {label}
      </button>
    {/each}

    <!-- External Grafana link -->
    <a
      href="/grafana/"
      target="_blank"
      rel="noopener"
      class="w-full flex items-center gap-3 px-4 py-2 text-sm text-gray-400 hover:bg-gray-800/50 hover:text-gray-200"
    >
      <span class="text-base">{'\u{1F4CA}'}</span>
      Monitoring
      <span class="text-xs text-gray-600 ml-auto">{'\u{2197}'}</span>
    </a>
  </nav>

  <!-- Footer -->
  <div class="p-3 border-t border-gray-800">
    <button
      class="w-full px-3 py-1.5 text-xs text-gray-400 hover:text-red-400 hover:bg-gray-800 rounded transition-colors"
      onclick={handleLogout}
    >
      Logout
    </button>
  </div>
</aside>

<!-- Mobile overlay -->
{#if mobileOpen}
  <!-- svelte-ignore a11y_click_events_have_key_events -->
  <!-- svelte-ignore a11y_no_static_element_interactions -->
  <div
    class="fixed inset-0 bg-black/50 z-30 md:hidden"
    onclick={() => mobileOpen = false}
  ></div>
{/if}
