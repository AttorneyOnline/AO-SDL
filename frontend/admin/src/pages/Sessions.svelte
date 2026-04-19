<script>
  import { get } from '../lib/api.js';

  let sessions = $state([]);
  let loading = $state(true);
  let selected = $state(null);

  async function refresh() {
    const res = await get('/admin/sessions');
    if (res.status === 200) sessions = res.data;
    loading = false;
  }

  $effect(() => {
    refresh();
    const interval = setInterval(refresh, 5000);
    return () => clearInterval(interval);
  });

  function formatBytes(b) {
    if (b < 1024) return b + ' B';
    if (b < 1024 * 1024) return (b / 1024).toFixed(1) + ' KB';
    return (b / (1024 * 1024)).toFixed(1) + ' MB';
  }
</script>

<div class="space-y-4">
  <h2 class="text-lg font-semibold">Players <span class="text-sm text-(--color-text-muted) font-normal">({sessions.length})</span></h2>

  {#if loading}
    <p class="text-(--color-text-muted) text-sm">Loading...</p>
  {:else}
    <div class="bg-(--color-surface-1) border border-(--color-border) overflow-hidden">
      <div class="overflow-x-auto">
        <table class="w-full text-sm">
          <thead>
            <tr class="text-left text-[10px] uppercase tracking-wider text-(--color-text-muted) border-b border-(--color-border)">
              <th class="px-4 py-2">Name</th>
              <th class="px-4 py-2">Protocol</th>
              <th class="px-4 py-2">Area</th>
              <th class="px-4 py-2 hidden sm:table-cell">Client</th>
              <th class="px-4 py-2 text-right hidden md:table-cell">Sent</th>
              <th class="px-4 py-2 text-right hidden md:table-cell">Recv</th>
              <th class="px-4 py-2 text-right">Idle</th>
            </tr>
          </thead>
          <tbody class="divide-y divide-(--color-border)/50">
            {#each sessions as s}
              <tr
                class="hover:bg-(--color-surface-2)/50 cursor-pointer {selected?.session_id === s.session_id ? 'bg-(--color-surface-2)' : ''}"
                onclick={() => selected = selected?.session_id === s.session_id ? null : s}
              >
                <td class="px-4 py-1.5 font-medium">{s.display_name || '(anon)'}</td>
                <td class="px-4 py-1.5 text-(--color-text-secondary)">{s.protocol}</td>
                <td class="px-4 py-1.5 text-(--color-text-secondary)">{s.area}</td>
                <td class="px-4 py-1.5 text-(--color-text-muted) hidden sm:table-cell truncate max-w-32">{s.client_software}</td>
                <td class="px-4 py-1.5 text-right text-(--color-text-muted) hidden md:table-cell tabular-nums">{formatBytes(s.bytes_sent)}</td>
                <td class="px-4 py-1.5 text-right text-(--color-text-muted) hidden md:table-cell tabular-nums">{formatBytes(s.bytes_received)}</td>
                <td class="px-4 py-1.5 text-right text-(--color-text-muted) tabular-nums">{s.idle_seconds}s</td>
              </tr>
            {/each}
          </tbody>
        </table>
      </div>
    </div>

    {#if selected}
      <div class="bg-(--color-surface-1) border border-(--color-border) p-4 space-y-3">
        <h3 class="text-sm font-semibold">{selected.display_name || '(anonymous)'}</h3>
        <div class="grid grid-cols-2 sm:grid-cols-3 lg:grid-cols-4 gap-3 text-xs">
          {@render detail('Session ID', selected.session_id)}
          {@render detail('Protocol', selected.protocol)}
          {@render detail('Area', selected.area)}
          {@render detail('Character', selected.character_id >= 0 ? '#' + selected.character_id : 'None')}
          {@render detail('HDID', selected.hardware_id)}
          {@render detail('Client', selected.client_software)}
          {@render detail('Packets Sent', selected.packets_sent)}
          {@render detail('Packets Recv', selected.packets_received)}
          {@render detail('Mod Actions', selected.mod_actions)}
          {@render detail('Bytes Sent', formatBytes(selected.bytes_sent))}
          {@render detail('Bytes Recv', formatBytes(selected.bytes_received))}
          {@render detail('Idle', selected.idle_seconds + 's')}
        </div>
      </div>
    {/if}
  {/if}
</div>

{#snippet detail(label, value)}
  <div>
    <div class="text-(--color-text-muted) text-[10px] uppercase tracking-wider">{label}</div>
    <div class="text-(--color-text-primary) font-mono mt-0.5 truncate">{value}</div>
  </div>
{/snippet}
