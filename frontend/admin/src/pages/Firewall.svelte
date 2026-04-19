<script>
  import { get } from '../lib/api.js';

  let tab = $state('firewall');
  let firewallData = $state({ enabled: false, rules: [] });
  let asnData = $state([]);
  let loading = $state(true);

  async function loadFirewall() { loading = true; const r = await get('/admin/firewall'); if (r.status === 200) firewallData = r.data; loading = false; }
  async function loadASN() { loading = true; const r = await get('/admin/asn-reputation'); if (r.status === 200) asnData = r.data?.asn_entries || []; loading = false; }

  $effect(() => { if (tab === 'firewall') loadFirewall(); else loadASN(); });

  function fmtTime(ts) { return ts ? new Date(ts * 1000).toLocaleString() : 'Never'; }
</script>

<div class="space-y-4">
  <h2 class="text-lg font-semibold">Firewall</h2>

  <div class="flex gap-4 border-b border-(--color-border)">
    <button onclick={() => tab = 'firewall'} class="pb-2 text-sm border-b-2 transition-colors {tab === 'firewall' ? 'border-(--color-accent) text-(--color-accent)' : 'border-transparent text-(--color-text-muted)'}">IP Rules</button>
    <button onclick={() => tab = 'asn'} class="pb-2 text-sm border-b-2 transition-colors {tab === 'asn' ? 'border-(--color-accent) text-(--color-accent)' : 'border-transparent text-(--color-text-muted)'}">ASN Reputation</button>
  </div>

  {#if loading}
    <p class="text-(--color-text-muted) text-sm">Loading...</p>
  {:else if tab === 'firewall'}
    <div class="bg-(--color-surface-1) border border-(--color-border) p-4">
      <div class="flex items-center gap-2 mb-3">
        <span class="text-sm font-medium">nftables</span>
        <span class="text-[10px] px-1 py-px font-medium {firewallData.enabled ? 'bg-emerald-500/15 text-emerald-400' : 'bg-(--color-surface-3) text-(--color-text-muted)'}">
          {firewallData.enabled ? 'Enabled' : 'Disabled'}
        </span>
        <span class="text-xs text-(--color-text-muted)">{firewallData.rules.length} rules</span>
      </div>
      {#if firewallData.rules.length > 0}
        <table class="w-full text-sm">
          <thead>
            <tr class="text-left text-[10px] uppercase tracking-wider text-(--color-text-muted) border-b border-(--color-border)">
              <th class="px-3 py-2">Target</th>
              <th class="px-3 py-2">Reason</th>
              <th class="px-3 py-2">Expires</th>
            </tr>
          </thead>
          <tbody class="divide-y divide-(--color-border)/50">
            {#each firewallData.rules as r}
              <tr class="hover:bg-(--color-surface-2)/50">
                <td class="px-3 py-1.5 font-mono text-xs">{r.target}</td>
                <td class="px-3 py-1.5 text-(--color-text-secondary) max-w-xs truncate">{r.reason}</td>
                <td class="px-3 py-1.5 text-xs">{r.expires_at === 0 ? 'Permanent' : fmtTime(r.expires_at)}</td>
              </tr>
            {/each}
          </tbody>
        </table>
      {:else}
        <p class="text-sm text-(--color-text-muted)">No active rules.</p>
      {/if}
    </div>
  {:else}
    <div class="bg-(--color-surface-1) border border-(--color-border) overflow-hidden">
      <div class="overflow-x-auto">
        <table class="w-full text-sm">
          <thead>
            <tr class="text-left text-[10px] uppercase tracking-wider text-(--color-text-muted) border-b border-(--color-border)">
              <th class="px-4 py-2">ASN</th>
              <th class="px-4 py-2">Organization</th>
              <th class="px-4 py-2">Status</th>
              <th class="px-4 py-2 hidden md:table-cell">Events</th>
              <th class="px-4 py-2 hidden md:table-cell">IPs</th>
            </tr>
          </thead>
          <tbody class="divide-y divide-(--color-border)/50">
            {#each asnData as e}
              <tr class="hover:bg-(--color-surface-2)/50">
                <td class="px-4 py-1.5 font-mono text-xs">AS{e.asn}</td>
                <td class="px-4 py-1.5">{e.as_org}</td>
                <td class="px-4 py-1.5">
                  <span class="text-[10px] px-1 py-px font-medium
                    {e.status === 'blocked' ? 'bg-red-500/15 text-red-400' :
                     e.status === 'rate_limited' ? 'bg-orange-500/15 text-orange-400' :
                     e.status === 'watched' ? 'bg-amber-500/15 text-amber-400' :
                     'bg-(--color-surface-3) text-(--color-text-muted)'}">
                    {e.status}
                  </span>
                </td>
                <td class="px-4 py-1.5 text-(--color-text-muted) hidden md:table-cell tabular-nums">{e.total_abuse_events}</td>
                <td class="px-4 py-1.5 text-(--color-text-muted) hidden md:table-cell tabular-nums">{e.abusive_ips}</td>
              </tr>
            {:else}
              <tr><td colspan="5" class="px-4 py-8 text-center text-(--color-text-muted)">No flagged ASNs</td></tr>
            {/each}
          </tbody>
        </table>
      </div>
    </div>
  {/if}
</div>
