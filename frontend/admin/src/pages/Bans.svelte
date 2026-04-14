<script>
  import { get, post } from '../lib/api.js';

  let bans = $state([]);
  let query = $state('');
  let loading = $state(true);

  async function refresh() {
    const params = query ? `?query=${encodeURIComponent(query)}&limit=100` : '?limit=100';
    const res = await get('/admin/bans' + params);
    if (res.status === 200) bans = res.data?.bans || [];
    loading = false;
  }

  $effect(() => { refresh(); });

  async function unban(ipid) {
    if (!confirm(`Unban ${ipid}?`)) return;
    await post('/moderation/actions', { action: 'unban', target: ipid });
    refresh();
  }

  function formatDuration(d) {
    if (d === -2) return 'Permanent';
    if (d === 0) return 'Invalidated';
    const h = Math.floor(d / 3600);
    const m = Math.floor((d % 3600) / 60);
    return h > 0 ? `${h}h ${m}m` : `${m}m`;
  }

  function formatTime(ts) {
    return ts ? new Date(ts * 1000).toLocaleString() : '';
  }
</script>

<div class="space-y-4">
  <div class="flex items-center justify-between flex-wrap gap-2">
    <h2 class="text-2xl font-bold">Bans</h2>
    <form onsubmit={(e) => { e.preventDefault(); refresh(); }} class="flex gap-2">
      <input
        type="text"
        bind:value={query}
        placeholder="Search by IPID, HDID, reason..."
        class="px-3 py-1.5 text-sm bg-gray-800 border border-gray-700 rounded-lg text-white placeholder-gray-500 w-64"
      />
      <button type="submit" class="px-3 py-1.5 text-sm bg-gray-700 hover:bg-gray-600 rounded-lg">Search</button>
    </form>
  </div>

  {#if loading}
    <p class="text-gray-500">Loading...</p>
  {:else}
    <div class="bg-gray-900 rounded-xl border border-gray-800 overflow-hidden">
      <div class="overflow-x-auto">
        <table class="w-full text-sm">
          <thead>
            <tr class="text-left text-xs text-gray-500 border-b border-gray-800">
              <th class="px-4 py-2">IPID</th>
              <th class="px-4 py-2 hidden sm:table-cell">HDID</th>
              <th class="px-4 py-2">Reason</th>
              <th class="px-4 py-2 hidden md:table-cell">Moderator</th>
              <th class="px-4 py-2 hidden lg:table-cell">When</th>
              <th class="px-4 py-2">Duration</th>
              <th class="px-4 py-2"></th>
            </tr>
          </thead>
          <tbody class="divide-y divide-gray-800/50">
            {#each bans as b}
              <tr class="hover:bg-gray-800/30">
                <td class="px-4 py-2 font-mono text-xs">{b.ipid}</td>
                <td class="px-4 py-2 font-mono text-xs text-gray-500 hidden sm:table-cell">{b.hdid}</td>
                <td class="px-4 py-2 max-w-xs truncate">{b.reason}</td>
                <td class="px-4 py-2 text-gray-400 hidden md:table-cell">{b.moderator}</td>
                <td class="px-4 py-2 text-gray-500 text-xs hidden lg:table-cell">{formatTime(b.timestamp)}</td>
                <td class="px-4 py-2">
                  <span class="text-xs px-1.5 py-0.5 rounded {b.permanent ? 'bg-red-900/50 text-red-300' : 'bg-gray-800 text-gray-400'}">
                    {formatDuration(b.duration)}
                  </span>
                </td>
                <td class="px-4 py-2">
                  <button
                    onclick={() => unban(b.ipid)}
                    class="text-xs text-red-400 hover:text-red-300"
                  >Unban</button>
                </td>
              </tr>
            {:else}
              <tr><td colspan="7" class="px-4 py-8 text-center text-gray-500">No bans found</td></tr>
            {/each}
          </tbody>
        </table>
      </div>
    </div>
  {/if}
</div>
