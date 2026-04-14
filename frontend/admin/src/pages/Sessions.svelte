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
  <h2 class="text-2xl font-bold">Sessions <span class="text-base text-gray-500 font-normal">({sessions.length})</span></h2>

  {#if loading}
    <p class="text-gray-500">Loading...</p>
  {:else}
    <div class="bg-gray-900 rounded-xl border border-gray-800 overflow-hidden">
      <div class="overflow-x-auto">
        <table class="w-full text-sm">
          <thead>
            <tr class="text-left text-xs text-gray-500 border-b border-gray-800">
              <th class="px-4 py-2">Name</th>
              <th class="px-4 py-2">Protocol</th>
              <th class="px-4 py-2">Area</th>
              <th class="px-4 py-2 hidden sm:table-cell">Client</th>
              <th class="px-4 py-2 text-right hidden md:table-cell">Sent</th>
              <th class="px-4 py-2 text-right hidden md:table-cell">Recv</th>
              <th class="px-4 py-2 text-right">Idle</th>
            </tr>
          </thead>
          <tbody class="divide-y divide-gray-800/50">
            {#each sessions as s}
              <tr
                class="hover:bg-gray-800/30 cursor-pointer {selected?.session_id === s.session_id ? 'bg-gray-800/50' : ''}"
                onclick={() => selected = selected?.session_id === s.session_id ? null : s}
              >
                <td class="px-4 py-2 font-medium">{s.display_name || '(anonymous)'}</td>
                <td class="px-4 py-2 text-gray-400">{s.protocol}</td>
                <td class="px-4 py-2 text-gray-400">{s.area}</td>
                <td class="px-4 py-2 text-gray-500 hidden sm:table-cell truncate max-w-32">{s.client_software}</td>
                <td class="px-4 py-2 text-right text-gray-500 hidden md:table-cell tabular-nums">{formatBytes(s.bytes_sent)}</td>
                <td class="px-4 py-2 text-right text-gray-500 hidden md:table-cell tabular-nums">{formatBytes(s.bytes_received)}</td>
                <td class="px-4 py-2 text-right text-gray-500 tabular-nums">{s.idle_seconds}s</td>
              </tr>
            {/each}
          </tbody>
        </table>
      </div>
    </div>

    <!-- Detail panel -->
    {#if selected}
      <div class="bg-gray-900 rounded-xl border border-gray-800 p-4 space-y-3">
        <h3 class="text-sm font-semibold">{selected.display_name || '(anonymous)'}</h3>
        <div class="grid grid-cols-2 sm:grid-cols-3 gap-3 text-xs">
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
    <div class="text-gray-500">{label}</div>
    <div class="text-gray-200 font-mono truncate">{value}</div>
  </div>
{/snippet}
