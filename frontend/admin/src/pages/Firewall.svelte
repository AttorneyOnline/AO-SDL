<script>
  import { get } from '../lib/api.js';

  let tab = $state('firewall');
  let firewallData = $state({ enabled: false, rules: [] });
  let asnData = $state([]);
  let loading = $state(true);

  async function loadFirewall() {
    loading = true;
    const res = await get('/admin/firewall');
    if (res.status === 200) firewallData = res.data;
    loading = false;
  }

  async function loadASN() {
    loading = true;
    const res = await get('/admin/asn-reputation');
    if (res.status === 200) asnData = res.data?.asn_entries || [];
    loading = false;
  }

  $effect(() => {
    if (tab === 'firewall') loadFirewall();
    else loadASN();
  });

  function formatTime(ts) {
    return ts ? new Date(ts * 1000).toLocaleString() : 'Never';
  }
</script>

<div class="space-y-4">
  <h2 class="text-2xl font-bold">Firewall &amp; Network</h2>

  <div class="flex gap-1 border-b border-gray-800">
    <button onclick={() => tab = 'firewall'} class="px-4 py-2 text-sm {tab === 'firewall' ? 'text-white border-b-2 border-blue-500' : 'text-gray-400'}">
      IP Rules
    </button>
    <button onclick={() => tab = 'asn'} class="px-4 py-2 text-sm {tab === 'asn' ? 'text-white border-b-2 border-blue-500' : 'text-gray-400'}">
      ASN Reputation
    </button>
  </div>

  {#if loading}
    <p class="text-gray-500">Loading...</p>
  {:else if tab === 'firewall'}
    <div class="bg-gray-900 rounded-xl border border-gray-800 p-4">
      <div class="flex items-center gap-2 mb-3">
        <span class="text-sm font-semibold">nftables</span>
        <span class="text-xs px-1.5 py-0.5 rounded {firewallData.enabled ? 'bg-green-900/50 text-green-300' : 'bg-gray-800 text-gray-500'}">
          {firewallData.enabled ? 'Enabled' : 'Disabled'}
        </span>
        <span class="text-xs text-gray-500">{firewallData.rules.length} rule(s)</span>
      </div>
      {#if firewallData.rules.length > 0}
        <table class="w-full text-sm">
          <thead>
            <tr class="text-left text-xs text-gray-500 border-b border-gray-800">
              <th class="px-3 py-2">Target</th>
              <th class="px-3 py-2">Reason</th>
              <th class="px-3 py-2 hidden md:table-cell">Installed</th>
              <th class="px-3 py-2">Expires</th>
            </tr>
          </thead>
          <tbody class="divide-y divide-gray-800/50">
            {#each firewallData.rules as r}
              <tr class="hover:bg-gray-800/30">
                <td class="px-3 py-2 font-mono text-xs">{r.target}</td>
                <td class="px-3 py-2 text-gray-400 max-w-xs truncate">{r.reason}</td>
                <td class="px-3 py-2 text-gray-500 text-xs hidden md:table-cell">{formatTime(r.installed_at)}</td>
                <td class="px-3 py-2 text-xs">{r.expires_at === 0 ? 'Permanent' : formatTime(r.expires_at)}</td>
              </tr>
            {/each}
          </tbody>
        </table>
      {:else}
        <p class="text-sm text-gray-500">No active firewall rules.</p>
      {/if}
    </div>
  {:else}
    <div class="bg-gray-900 rounded-xl border border-gray-800 overflow-hidden">
      <div class="overflow-x-auto">
        <table class="w-full text-sm">
          <thead>
            <tr class="text-left text-xs text-gray-500 border-b border-gray-800">
              <th class="px-4 py-2">ASN</th>
              <th class="px-4 py-2">Organization</th>
              <th class="px-4 py-2">Status</th>
              <th class="px-4 py-2 hidden md:table-cell">Abuse Events</th>
              <th class="px-4 py-2 hidden md:table-cell">Abusive IPs</th>
              <th class="px-4 py-2 hidden lg:table-cell">Block Reason</th>
            </tr>
          </thead>
          <tbody class="divide-y divide-gray-800/50">
            {#each asnData as e}
              <tr class="hover:bg-gray-800/30">
                <td class="px-4 py-2 font-mono text-xs">AS{e.asn}</td>
                <td class="px-4 py-2 text-gray-300">{e.as_org}</td>
                <td class="px-4 py-2">
                  <span class="text-xs px-1.5 py-0.5 rounded
                    {e.status === 'blocked' ? 'bg-red-900/50 text-red-300' :
                     e.status === 'rate_limited' ? 'bg-orange-900/50 text-orange-300' :
                     e.status === 'watched' ? 'bg-yellow-900/50 text-yellow-300' :
                     'bg-gray-800 text-gray-400'}">
                    {e.status}
                  </span>
                </td>
                <td class="px-4 py-2 text-gray-500 hidden md:table-cell">{e.total_abuse_events}</td>
                <td class="px-4 py-2 text-gray-500 hidden md:table-cell">{e.abusive_ips}</td>
                <td class="px-4 py-2 text-gray-500 text-xs max-w-xs truncate hidden lg:table-cell">{e.block_reason}</td>
              </tr>
            {:else}
              <tr><td colspan="6" class="px-4 py-8 text-center text-gray-500">No flagged ASNs</td></tr>
            {/each}
          </tbody>
        </table>
      </div>
    </div>
  {/if}
</div>
