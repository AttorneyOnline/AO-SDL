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

  function metric(name) {
    return metrics.get(name) ?? 0;
  }
</script>

<div class="space-y-6">
  <h2 class="text-2xl font-bold">Dashboard</h2>

  {#if loading}
    <p class="text-gray-500">Loading...</p>
  {:else}
    <!-- Server info banner -->
    {#if serverInfo}
      <div class="bg-gray-900 rounded-xl border border-gray-800 p-4">
        <div class="flex flex-wrap items-baseline gap-x-4 gap-y-1">
          <span class="text-lg font-semibold">{serverInfo.name}</span>
          <span class="text-xs text-gray-500">v{serverInfo.version}</span>
          {#if serverInfo.description}
            <span class="text-sm text-gray-400">&mdash; {serverInfo.description}</span>
          {/if}
        </div>
      </div>
    {/if}

    <!-- Gauge cards -->
    <div class="grid grid-cols-2 sm:grid-cols-3 lg:grid-cols-5 gap-3">
      {@render gauge('Players', serverInfo?.online ?? 0, '/' + (serverInfo?.max ?? '?'))}
      {@render gauge('AO2 Clients', metric('kagami_ws_connections'), '')}
      {@render gauge('REST Sessions', sessions.length, '')}
      {@render gauge('Areas', areas.length, '')}
      {@render gauge('Moderators', metric('kagami_sessions_moderators'), '')}
    </div>

    <!-- Area summary -->
    <div class="bg-gray-900 rounded-xl border border-gray-800 overflow-hidden">
      <div class="px-4 py-3 border-b border-gray-800">
        <h3 class="text-sm font-semibold text-gray-300">Areas</h3>
      </div>
      <div class="divide-y divide-gray-800">
        {#each areas as area}
          <div class="px-4 py-2 flex items-center justify-between text-sm">
            <div>
              <span class="font-medium">{area.name}</span>
              {#if area.status && area.status !== 'IDLE'}
                <span class="ml-2 text-xs px-1.5 py-0.5 rounded bg-blue-900/50 text-blue-300">{area.status}</span>
              {/if}
              {#if area.locked && area.locked !== 'FREE'}
                <span class="ml-1 text-xs px-1.5 py-0.5 rounded bg-yellow-900/50 text-yellow-300">{area.locked}</span>
              {/if}
            </div>
            <span class="text-gray-500">{area.players ?? 0} players</span>
          </div>
        {/each}
      </div>
    </div>

    <!-- Session summary table -->
    {#if sessions.length > 0}
      <div class="bg-gray-900 rounded-xl border border-gray-800 overflow-hidden">
        <div class="px-4 py-3 border-b border-gray-800">
          <h3 class="text-sm font-semibold text-gray-300">Active Sessions</h3>
        </div>
        <div class="overflow-x-auto">
          <table class="w-full text-sm">
            <thead>
              <tr class="text-left text-xs text-gray-500 border-b border-gray-800">
                <th class="px-4 py-2">Name</th>
                <th class="px-4 py-2">Protocol</th>
                <th class="px-4 py-2">Area</th>
                <th class="px-4 py-2 text-right">Idle</th>
              </tr>
            </thead>
            <tbody class="divide-y divide-gray-800/50">
              {#each sessions.slice(0, 20) as s}
                <tr class="hover:bg-gray-800/30">
                  <td class="px-4 py-1.5">{s.display_name || '(anonymous)'}</td>
                  <td class="px-4 py-1.5 text-gray-400">{s.protocol}</td>
                  <td class="px-4 py-1.5 text-gray-400">{s.area}</td>
                  <td class="px-4 py-1.5 text-right text-gray-500">{s.idle_seconds}s</td>
                </tr>
              {/each}
            </tbody>
          </table>
        </div>
        {#if sessions.length > 20}
          <div class="px-4 py-2 text-xs text-gray-500 border-t border-gray-800">
            Showing 20 of {sessions.length} &mdash;
            <a href="#/sessions" class="text-blue-400 hover:underline">View all</a>
          </div>
        {/if}
      </div>
    {/if}
  {/if}
</div>

{#snippet gauge(label, value, suffix)}
  <div class="bg-gray-900 rounded-xl border border-gray-800 p-4">
    <div class="text-2xl font-bold tabular-nums">
      {value}<span class="text-sm text-gray-500 font-normal">{suffix}</span>
    </div>
    <div class="text-xs text-gray-500 mt-1">{label}</div>
  </div>
{/snippet}
