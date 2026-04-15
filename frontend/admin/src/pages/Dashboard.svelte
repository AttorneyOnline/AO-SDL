<script>
  import { get, fetchMetrics } from '../lib/api.js';

  let serverInfo = $state(null);
  let sessions = $state([]);
  let areas = $state([]);
  let metrics = $state(new Map());
  let loading = $state(true);

  async function refresh() {
    const [serverRes, sessionsRes, areasRes, metricsData] = await Promise.all([
      get('/server'),
      get('/admin/sessions'),
      get('/areas'),
      fetchMetrics(),
    ]);
    if (serverRes.status === 200) serverInfo = serverRes.data;
    if (sessionsRes.status === 200) sessions = sessionsRes.data;
    if (areasRes.status === 200) areas = areasRes.data?.areas || [];
    metrics = metricsData;
    loading = false;
  }

  $effect(() => {
    refresh();
    const interval = setInterval(refresh, 10000);
    return () => clearInterval(interval);
  });

  function m(name) { return metrics.get(name) ?? 0; }
</script>

<div class="space-y-5">
  <h2 class="text-lg font-semibold">Dashboard</h2>

  {#if loading}
    <p class="text-(--color-text-muted) text-sm">Loading...</p>
  {:else}
    {#if serverInfo}
      <div class="bg-(--color-surface-1) border border-(--color-border) px-4 py-3 flex flex-wrap items-baseline gap-x-3 gap-y-1">
        <span class="font-semibold text-sm">{serverInfo.name}</span>
        <span class="text-xs text-(--color-text-muted)">v{serverInfo.version}</span>
        {#if serverInfo.description}
          <span class="text-xs text-(--color-text-secondary)">{serverInfo.description}</span>
        {/if}
      </div>
    {/if}

    <div class="grid grid-cols-2 sm:grid-cols-3 lg:grid-cols-6 gap-px bg-(--color-border)">
      {@render gauge('Players', serverInfo?.online ?? 0, '/' + (serverInfo?.max ?? '?'))}
      {@render gauge('WS Clients', m('kagami_ws_connections'), '')}
      {@render gauge('REST', sessions.length, '')}
      {@render gauge('Areas', areas.length, '')}
      {@render gauge('Mods', m('kagami_sessions_moderators'), '')}
      {@render gauge('Chars', m('kagami_characters_taken'), '')}
    </div>

    <div class="grid lg:grid-cols-2 gap-5">
      <!-- Areas -->
      <div class="bg-(--color-surface-1) border border-(--color-border) overflow-hidden">
        <div class="px-4 py-2 border-b border-(--color-border)">
          <h3 class="text-xs font-semibold uppercase tracking-wide text-(--color-text-muted)">Areas</h3>
        </div>
        <div class="divide-y divide-(--color-border)">
          {#each areas as area}
            <div class="px-4 py-1.5 flex items-center justify-between text-sm">
              <div class="flex items-center gap-2">
                <span>{area.name}</span>
                {#if area.status && area.status !== 'IDLE'}
                  <span class="text-[10px] px-1 py-px bg-cyan-500/15 text-cyan-400 font-medium">{area.status}</span>
                {/if}
                {#if area.locked && area.locked !== 'FREE'}
                  <span class="text-[10px] px-1 py-px bg-amber-500/15 text-amber-400 font-medium">{area.locked}</span>
                {/if}
              </div>
              <span class="text-xs text-(--color-text-muted) tabular-nums">{area.players ?? 0}</span>
            </div>
          {/each}
        </div>
      </div>

      <!-- Sessions -->
      <div class="bg-(--color-surface-1) border border-(--color-border) overflow-hidden">
        <div class="px-4 py-2 border-b border-(--color-border) flex items-center justify-between">
          <h3 class="text-xs font-semibold uppercase tracking-wide text-(--color-text-muted)">Active Sessions</h3>
          {#if sessions.length > 15}
            <a href="#/sessions" class="text-xs text-(--color-text-muted) hover:text-(--color-text-primary)">View all</a>
          {/if}
        </div>
        <div class="overflow-x-auto">
          <table class="w-full text-sm">
            <thead>
              <tr class="text-left text-[10px] uppercase tracking-wider text-(--color-text-muted) border-b border-(--color-border)">
                <th class="px-4 py-1.5">Name</th>
                <th class="px-4 py-1.5">Area</th>
                <th class="px-4 py-1.5 text-right">Idle</th>
              </tr>
            </thead>
            <tbody class="divide-y divide-(--color-border)/50">
              {#each sessions.slice(0, 15) as s}
                <tr class="hover:bg-(--color-surface-2)/50">
                  <td class="px-4 py-1">{s.display_name || '(anon)'}</td>
                  <td class="px-4 py-1 text-(--color-text-secondary)">{s.area}</td>
                  <td class="px-4 py-1 text-right text-(--color-text-muted) tabular-nums">{s.idle_seconds}s</td>
                </tr>
              {:else}
                <tr><td colspan="3" class="px-4 py-4 text-center text-(--color-text-muted) text-xs">No active sessions</td></tr>
              {/each}
            </tbody>
          </table>
        </div>
      </div>
    </div>
  {/if}
</div>

{#snippet gauge(label, value, suffix)}
  <div class="bg-(--color-surface-1) p-3">
    <div class="text-xl font-semibold tabular-nums">
      {value}<span class="text-xs text-(--color-text-muted) font-normal">{suffix}</span>
    </div>
    <div class="text-[10px] uppercase tracking-wider text-(--color-text-muted) mt-0.5">{label}</div>
  </div>
{/snippet}
