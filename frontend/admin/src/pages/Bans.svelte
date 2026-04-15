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

  function fmtDur(d) {
    if (d === -2) return 'Permanent';
    if (d === 0) return 'Invalidated';
    const h = Math.floor(d / 3600);
    const m = Math.floor((d % 3600) / 60);
    return h > 0 ? `${h}h ${m}m` : `${m}m`;
  }

  function fmtTime(ts) { return ts ? new Date(ts * 1000).toLocaleString() : ''; }
</script>

<div class="space-y-4">
  <div class="flex items-center justify-between flex-wrap gap-2">
    <h2 class="text-lg font-semibold">Bans</h2>
    <form onsubmit={(e) => { e.preventDefault(); refresh(); }} class="flex gap-px">
      <input
        type="text" bind:value={query}
        placeholder="Search IPID, HDID, reason..."
        class="px-3 py-1.5 text-sm bg-(--color-surface-2) border border-(--color-border) text-(--color-text-primary)
               placeholder:text-(--color-text-muted) focus:outline-none focus:border-(--color-border-active) w-56"
      />
      <button type="submit" class="px-3 py-1.5 text-sm bg-(--color-surface-3) border border-(--color-border) text-(--color-text-secondary) hover:text-(--color-text-primary)">Search</button>
    </form>
  </div>

  {#if loading}
    <p class="text-(--color-text-muted) text-sm">Loading...</p>
  {:else}
    <div class="bg-(--color-surface-1) border border-(--color-border) overflow-hidden">
      <div class="overflow-x-auto">
        <table class="w-full text-sm">
          <thead>
            <tr class="text-left text-[10px] uppercase tracking-wider text-(--color-text-muted) border-b border-(--color-border)">
              <th class="px-4 py-2">IPID</th>
              <th class="px-4 py-2">Reason</th>
              <th class="px-4 py-2 hidden md:table-cell">By</th>
              <th class="px-4 py-2 hidden lg:table-cell">When</th>
              <th class="px-4 py-2">Duration</th>
              <th class="px-4 py-2"></th>
            </tr>
          </thead>
          <tbody class="divide-y divide-(--color-border)/50">
            {#each bans as b}
              <tr class="hover:bg-(--color-surface-2)/50">
                <td class="px-4 py-1.5 font-mono text-xs">{b.ipid}</td>
                <td class="px-4 py-1.5 max-w-xs truncate">{b.reason}</td>
                <td class="px-4 py-1.5 text-(--color-text-secondary) hidden md:table-cell">{b.moderator}</td>
                <td class="px-4 py-1.5 text-(--color-text-muted) text-xs hidden lg:table-cell">{fmtTime(b.timestamp)}</td>
                <td class="px-4 py-1.5">
                  <span class="text-[10px] px-1 py-px font-medium {b.permanent ? 'bg-red-500/15 text-red-400' : 'bg-(--color-surface-3) text-(--color-text-muted)'}">
                    {fmtDur(b.duration)}
                  </span>
                </td>
                <td class="px-4 py-1.5">
                  <button onclick={() => unban(b.ipid)} class="text-xs text-red-400 hover:text-red-300">Unban</button>
                </td>
              </tr>
            {:else}
              <tr><td colspan="6" class="px-4 py-8 text-center text-(--color-text-muted) text-sm">No bans found</td></tr>
            {/each}
          </tbody>
        </table>
      </div>
    </div>
  {/if}
</div>
