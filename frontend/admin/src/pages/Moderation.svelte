<script>
  import { get, post } from '../lib/api.js';

  let tab = $state('audit');
  let events = $state([]);
  let mutes = $state([]);
  let loading = $state(true);
  let actionTarget = $state('');
  let actionReason = $state('');
  let actionResult = $state('');

  const ACTION_LABELS = {
    '0': 'none', '1': 'log', '2': 'censor', '3': 'drop',
    '4': 'mute', '5': 'kick', '6': 'ban', '7': 'perma_ban',
    'none': 'none', 'log': 'log', 'censor': 'censor', 'drop': 'drop',
    'mute': 'mute', 'kick': 'kick', 'ban': 'ban', 'perma_ban': 'perma_ban',
  };

  const ACTION_COLORS = {
    ban: 'bg-red-500/15 text-red-400', perma_ban: 'bg-red-500/15 text-red-400',
    kick: 'bg-amber-500/15 text-amber-400', mute: 'bg-orange-500/15 text-orange-400',
    censor: 'bg-violet-500/15 text-violet-400', drop: 'bg-violet-500/15 text-violet-400',
    log: 'bg-(--color-surface-3) text-(--color-text-muted)',
    none: 'bg-(--color-surface-3) text-(--color-text-muted)',
  };

  function actionLabel(raw) { return ACTION_LABELS[String(raw)] || String(raw); }
  function actionColor(raw) { return ACTION_COLORS[actionLabel(raw)] || 'bg-(--color-surface-3) text-(--color-text-muted)'; }

  async function loadAudit() { loading = true; const r = await get('/admin/moderation-events?limit=200'); if (r.status === 200) events = r.data?.events || []; loading = false; }
  async function loadMutes() { loading = true; const r = await get('/admin/mutes'); if (r.status === 200) mutes = r.data?.mutes || []; loading = false; }

  $effect(() => { if (tab === 'audit') loadAudit(); else loadMutes(); });

  async function doAction(action) {
    if (!actionTarget) return;
    actionResult = '';
    const body = { action, target: actionTarget, reason: actionReason || undefined };
    if (action === 'mute') body.duration = 900;
    const res = await post('/moderation/actions', body);
    actionResult = res.status === 200 ? `${action} applied` : (res.data?.reason || 'Failed');
    actionTarget = ''; actionReason = '';
    if (tab === 'mutes') loadMutes();
  }

  async function unmute(ipid) { await post('/moderation/actions', { action: 'unmute', target: ipid }); loadMutes(); }
</script>

<div class="space-y-4">
  <h2 class="text-lg font-semibold">Moderation</h2>

  <div class="bg-(--color-surface-1) border border-(--color-border) p-4">
    <h3 class="text-[10px] font-semibold uppercase tracking-wider text-(--color-text-muted) mb-3">Quick Action</h3>
    <div class="flex flex-wrap gap-px">
      <input bind:value={actionTarget} placeholder="IPID / session ID / IP" class="px-3 py-1.5 text-sm bg-(--color-surface-2) border border-(--color-border) text-(--color-text-primary) placeholder:text-(--color-text-muted) w-44 focus:outline-none focus:border-(--color-border-active)" />
      <input bind:value={actionReason} placeholder="Reason" class="px-3 py-1.5 text-sm bg-(--color-surface-2) border border-(--color-border) text-(--color-text-primary) placeholder:text-(--color-text-muted) flex-1 min-w-24 focus:outline-none focus:border-(--color-border-active)" />
      <button onclick={() => doAction('kick')} class="px-3 py-1.5 text-sm bg-amber-600 text-white hover:bg-amber-500">Kick</button>
      <button onclick={() => doAction('mute')} class="px-3 py-1.5 text-sm bg-orange-600 text-white hover:bg-orange-500">Mute</button>
      <button onclick={() => doAction('ban')} class="px-3 py-1.5 text-sm bg-red-600 text-white hover:bg-red-500">Ban</button>
    </div>
    {#if actionResult}<p class="text-xs mt-2 text-(--color-text-muted)">{actionResult}</p>{/if}
  </div>

  <div class="flex gap-4 border-b border-(--color-border)">
    <button onclick={() => tab = 'audit'} class="pb-2 text-sm border-b-2 transition-colors {tab === 'audit' ? 'border-(--color-accent) text-(--color-accent)' : 'border-transparent text-(--color-text-muted)'}">Audit Log</button>
    <button onclick={() => tab = 'mutes'} class="pb-2 text-sm border-b-2 transition-colors {tab === 'mutes' ? 'border-(--color-accent) text-(--color-accent)' : 'border-transparent text-(--color-text-muted)'}">Active Mutes</button>
  </div>

  {#if loading}
    <p class="text-(--color-text-muted) text-sm">Loading...</p>
  {:else if tab === 'audit'}
    <div class="bg-(--color-surface-1) border border-(--color-border) overflow-hidden">
      <div class="overflow-x-auto">
        <table class="w-full text-xs">
          <thead>
            <tr class="text-left text-[10px] uppercase tracking-wider text-(--color-text-muted) border-b border-(--color-border)">
              <th class="px-3 py-2">Time</th>
              <th class="px-3 py-2">IPID</th>
              <th class="px-3 py-2">Ch</th>
              <th class="px-3 py-2">Action</th>
              <th class="px-3 py-2 hidden md:table-cell">Message</th>
              <th class="px-3 py-2 hidden lg:table-cell">Reason</th>
            </tr>
          </thead>
          <tbody class="divide-y divide-(--color-border)/50">
            {#each events as e}
              <tr class="hover:bg-(--color-surface-2)/50">
                <td class="px-3 py-1.5 text-(--color-text-muted)">{new Date(e.timestamp_ms).toLocaleString()}</td>
                <td class="px-3 py-1.5 font-mono">{e.ipid}</td>
                <td class="px-3 py-1.5 text-(--color-text-secondary)">{e.channel}</td>
                <td class="px-3 py-1.5">
                  <span class="text-[10px] px-1 py-px font-medium {actionColor(e.action)}">{actionLabel(e.action)}</span>
                </td>
                <td class="px-3 py-1.5 text-(--color-text-secondary) max-w-xs truncate hidden md:table-cell">{e.message_sample}</td>
                <td class="px-3 py-1.5 text-(--color-text-muted) max-w-xs truncate hidden lg:table-cell">{e.reason}</td>
              </tr>
            {:else}
              <tr><td colspan="6" class="px-4 py-8 text-center text-(--color-text-muted)">No events</td></tr>
            {/each}
          </tbody>
        </table>
      </div>
    </div>
  {:else}
    <div class="bg-(--color-surface-1) border border-(--color-border) overflow-hidden">
      <div class="overflow-x-auto">
        <table class="w-full text-sm">
          <thead>
            <tr class="text-left text-[10px] uppercase tracking-wider text-(--color-text-muted) border-b border-(--color-border)">
              <th class="px-4 py-2">IPID</th>
              <th class="px-4 py-2">Reason</th>
              <th class="px-4 py-2">Remaining</th>
              <th class="px-4 py-2"></th>
            </tr>
          </thead>
          <tbody class="divide-y divide-(--color-border)/50">
            {#each mutes as m}
              <tr class="hover:bg-(--color-surface-2)/50">
                <td class="px-4 py-1.5 font-mono text-xs">{m.ipid}</td>
                <td class="px-4 py-1.5 text-(--color-text-secondary)">{m.reason}</td>
                <td class="px-4 py-1.5">{m.seconds_remaining < 0 ? 'Permanent' : Math.ceil(m.seconds_remaining / 60) + 'm'}</td>
                <td class="px-4 py-1.5"><button onclick={() => unmute(m.ipid)} class="text-xs text-red-400 hover:text-red-300">Unmute</button></td>
              </tr>
            {:else}
              <tr><td colspan="4" class="px-4 py-8 text-center text-(--color-text-muted)">No active mutes</td></tr>
            {/each}
          </tbody>
        </table>
      </div>
    </div>
  {/if}
</div>
