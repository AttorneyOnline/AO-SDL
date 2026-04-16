<script>
  import { auth, logout } from '../lib/auth.svelte.js';
  import {
    LayoutDashboard, Users, MessageSquare, Settings, Ban,
    Shield, Map, KeyRound, Flame, FolderOpen, Sun, Moon,
    ExternalLink, LogOut, Menu, X, Power
  } from 'lucide-svelte';
  import { post } from '../lib/api.js';

  let { currentPage = 'dashboard' } = $props();
  let mobileOpen = $state(false);
  let dark = $state(!document.documentElement.classList.contains('light'));

  function toggleTheme() {
    dark = !dark;
    document.documentElement.classList.toggle('light', !dark);
    localStorage.setItem('kagami_theme', dark ? 'dark' : 'light');
  }

  // Restore saved theme
  $effect(() => {
    const saved = localStorage.getItem('kagami_theme');
    if (saved === 'light') {
      dark = false;
      document.documentElement.classList.add('light');
    }
  });

  const links = [
    { page: 'dashboard', label: 'Dashboard', icon: LayoutDashboard },
    { page: 'sessions',  label: 'Players',   icon: Users },
    { page: 'traffic',   label: 'Traffic',   icon: MessageSquare },
    { page: 'config',    label: 'Config',    icon: Settings },
    { page: 'bans',      label: 'Bans',      icon: Ban },
    { page: 'moderation',label: 'Moderation', icon: Shield },
    { page: 'areas',     label: 'Areas',     icon: Map },
    { page: 'users',     label: 'Accounts',  icon: KeyRound },
    { page: 'firewall',  label: 'Firewall',  icon: Flame },
    { page: 'content',   label: 'Content',   icon: FolderOpen },
  ];

  function navigate(page) {
    window.location.hash = '#/' + page;
    mobileOpen = false;
  }

  async function handleLogout() {
    await logout();
    window.location.hash = '#/login';
  }

  async function handleStop() {
    if (!confirm('Shut down the server? All connections will be closed.')) return;
    await post('/admin/stop');
  }
</script>

{#snippet iconRender(Icon)}
  <Icon size={15} strokeWidth={1.5} />
{/snippet}

<!-- Mobile hamburger -->
<button
  class="md:hidden fixed top-3 left-3 z-50 p-2 bg-(--color-surface-2) border border-(--color-border)"
  onclick={() => mobileOpen = !mobileOpen}
  aria-label="Toggle navigation"
>
  {#if mobileOpen}
    <X size={18} />
  {:else}
    <Menu size={18} />
  {/if}
</button>

<!-- Sidebar -->
<aside
  class="fixed md:static inset-y-0 left-0 z-40 w-52 bg-(--color-surface-1) border-r border-(--color-border)
         flex flex-col transition-transform duration-150
         {mobileOpen ? 'translate-x-0' : '-translate-x-full'} md:translate-x-0"
>
  <div class="px-4 py-3 border-b border-(--color-border)">
    <h1 class="text-sm font-semibold tracking-wide uppercase text-(--color-accent)">Kagami</h1>
    <p class="text-xs text-(--color-text-muted) mt-0.5">{auth.username} &middot; {auth.acl}</p>
  </div>

  <nav class="flex-1 overflow-y-auto py-1">
    {#each links as { page, label, icon }}
      <button
        class="w-full flex items-center gap-2.5 px-4 py-1.5 text-sm transition-colors
               {currentPage === page
                 ? 'bg-(--color-surface-2) text-(--color-accent) font-medium'
                 : 'text-(--color-text-secondary) hover:bg-(--color-surface-2) hover:text-(--color-text-primary)'}"
        onclick={() => navigate(page)}
      >
        {@render iconRender(icon)}
        {label}
      </button>
    {/each}

    <a
      href="/grafana/"
      target="_blank"
      rel="noopener"
      class="w-full flex items-center gap-2.5 px-4 py-1.5 text-sm text-(--color-text-secondary)
             hover:bg-(--color-surface-2) hover:text-(--color-text-primary)"
    >
      <ExternalLink size={15} strokeWidth={1.5} />
      Monitoring
    </a>
  </nav>

  <div class="px-3 py-2 border-t border-(--color-border) flex items-center justify-between">
    <div class="flex items-center gap-1">
      <button onclick={handleLogout}
        class="flex items-center gap-1.5 px-2 py-1 text-xs text-(--color-text-muted) hover:text-(--color-text-primary) transition-colors">
        <LogOut size={13} strokeWidth={1.5} /> Logout
      </button>
      <button onclick={handleStop}
        class="flex items-center gap-1 px-2 py-1 text-xs text-(--color-text-muted) hover:text-red-500 transition-colors"
        title="Shut down server">
        <Power size={13} strokeWidth={1.5} />
      </button>
    </div>
    <button
      onclick={toggleTheme}
      class="p-1 text-(--color-text-muted) hover:text-(--color-text-primary) transition-colors"
      aria-label="Toggle theme"
    >
      {#if dark}
        <Sun size={14} strokeWidth={1.5} />
      {:else}
        <Moon size={14} strokeWidth={1.5} />
      {/if}
    </button>
  </div>
</aside>

{#if mobileOpen}
  <!-- svelte-ignore a11y_click_events_have_key_events -->
  <!-- svelte-ignore a11y_no_static_element_interactions -->
  <div class="fixed inset-0 bg-black/40 z-30 md:hidden" onclick={() => mobileOpen = false}></div>
{/if}
