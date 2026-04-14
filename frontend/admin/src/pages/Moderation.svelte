<script>
  import { get, post } from '../lib/api.js';

  let tab = $state('audit');
  let events = $state([]);
  let mutes = $state([]);
  let loading = $state(true);
  let actionTarget = $state('');
  let actionReason = $state('');
  let actionResult = $state('');

  async function loadAudit() {
    loading = true;
    const res = await get('/admin/moderation-events?limit=100');
    if (res.status === 200) events = res.data?.events || [];
    loading = false;
  }

  async function loadMutes() {
    loading = true;
    const res = await get('/admin/mutes');
    if (res.status === 200) mutes = res.data?.mutes || [];
    loading = false;
  }

  $effect(() => {
    if (tab === 'audit') loadAudit();
    else if (tab === 'mutes') loadMutes();
  });

  async function doAction(action) {
    if (!actionTarget) return;
    actionResult = '';
    const body = { action, target: actionTarget, reason: actionReason || undefined };
    if (action === 'mute') body.duration = 900; // 15 min default
    const res = await post('/moderation/actions', body);
    actionResult = res.status === 200 ? `${action} applied` : (res.data?.reason || 'Failed');
    actionTarget = '';
    actionReason = '';
    if (tab === 'mutes') loadMutes();
  }

  async function unmute(ipid) {
    await post('/moderation/actions', { action: 'unmute', target: ipid });
    loadMutes();
  }
</script>

<div class="space-y-4">
  <h2 class="text-2xl font-bold">Moderation</h2>

  <!-- Quick actions -->
  <div class="bg-gray-900 rounded-xl border border-gray-800 p-4">
    <h3 class="text-sm font-semibold text-gray-300 mb-3">Quick Action</h3>
    <div class="flex flex-wrap gap-2">
      <input bind:value={actionTarget} placeholder="IPID or session ID" class="px-3 py-1.5 text-sm bg-gray-800 border border-gray-700 rounded-lg text-white placeholder-gray-500 w-40" />
      <input bind:value={actionReason} placeholder="Reason (optional)" class="px-3 py-1.5 text-sm bg-gray-800 border border-gray-700 rounded-lg text-white placeholder-gray-500 flex-1 min-w-32" />
      <button onclick={() => doAction('kick')} class="px-3 py-1.5 text-sm bg-yellow-700 hover:bg-yellow-600 rounded-lg">Kick</button>
      <button onclick={() => doAction('mute')} class="px-3 py-1.5 text-sm bg-orange-700 hover:bg-orange-600 rounded-lg">Mute</button>
      <button onclick={() => doAction('ban')} class="px-3 py-1.5 text-sm bg-red-700 hover:bg-red-600 rounded-lg">Ban</button>
    </div>
    {#if actionResult}
      <p class="text-xs mt-2 text-gray-400">{actionResult}</p>
    {/if}
  </div>

  <!-- Tabs -->
  <div class="flex gap-1 border-b border-gray-800">
    <button onclick={() => tab = 'audit'} class="px-4 py-2 text-sm {tab === 'audit' ? 'text-white border-b-2 border-blue-500' : 'text-gray-400'}">Audit Log</button>
    <button onclick={() => tab = 'mutes'} class="px-4 py-2 text-sm {tab === 'mutes' ? 'text-white border-b-2 border-blue-500' : 'text-gray-400'}">Active Mutes</button>
  </div>

  {#if loading}
    <p class="text-gray-500">Loading...</p>
  {:else if tab === 'audit'}
    <div class="bg-gray-900 rounded-xl border border-gray-800 overflow-hidden">
      <div class="overflow-x-auto">
        <table class="w-full text-xs">
          <thead>
            <tr class="text-left text-gray-500 border-b border-gray-800">
              <th class="px-3 py-2">Time</th>
              <th class="px-3 py-2">IPID</th>
              <th class="px-3 py-2">Channel</th>
              <th class="px-3 py-2">Action</th>
              <th class="px-3 py-2 hidden md:table-cell">Message</th>
              <th class="px-3 py-2 hidden lg:table-cell">Reason</th>
            </tr>
          </thead>
          <tbody class="divide-y divide-gray-800/50">
            {#each events as e}
              <tr class="hover:bg-gray-800/30">
                <td class="px-3 py-1.5 text-gray-500">{new Date(e.timestamp_ms).toLocaleString()}</td>
                <td class="px-3 py-1.5 font-mono">{e.ipid}</td>
                <td class="px-3 py-1.5 text-gray-400">{e.channel}</td>
                <td class="px-3 py-1.5">
                  <span class="px-1.5 py-0.5 rounded text-xs
                    {e.action === 'ban' || e.action === 'perma_ban' ? 'bg-red-900/50 text-red-300' :
                     e.action === 'kick' ? 'bg-yellow-900/50 text-yellow-300' :
                     e.action === 'mute' ? 'bg-orange-900/50 text-orange-300' :
                     e.action === 'censor' ? 'bg-purple-900/50 text-purple-300' :
                     'bg-gray-800 text-gray-400'}">
                    {e.action}
                  </span>
                </td>
                <td class="px-3 py-1.5 text-gray-400 max-w-xs truncate hidden md:table-cell">{e.message_sample}</td>
                <td class="px-3 py-1.5 text-gray-500 max-w-xs truncate hidden lg:table-cell">{e.reason}</td>
              </tr>
            {:else}
              <tr><td colspan="6" class="px-4 py-8 text-center text-gray-500">No moderation events</td></tr>
            {/each}
          </tbody>
        </table>
      </div>
    </div>
  {:else if tab === 'mutes'}
    <div class="bg-gray-900 rounded-xl border border-gray-800 overflow-hidden">
      <div class="overflow-x-auto">
        <table class="w-full text-sm">
          <thead>
            <tr class="text-left text-xs text-gray-500 border-b border-gray-800">
              <th class="px-4 py-2">IPID</th>
              <th class="px-4 py-2">Reason</th>
              <th class="px-4 py-2">Remaining</th>
              <th class="px-4 py-2"></th>
            </tr>
          </thead>
          <tbody class="divide-y divide-gray-800/50">
            {#each mutes as m}
              <tr class="hover:bg-gray-800/30">
                <td class="px-4 py-2 font-mono text-xs">{m.ipid}</td>
                <td class="px-4 py-2 text-gray-400">{m.reason}</td>
                <td class="px-4 py-2">{m.seconds_remaining < 0 ? 'Permanent' : Math.ceil(m.seconds_remaining / 60) + 'm'}</td>
                <td class="px-4 py-2">
                  <button onclick={() => unmute(m.ipid)} class="text-xs text-red-400 hover:text-red-300">Unmute</button>
                </td>
              </tr>
            {:else}
              <tr><td colspan="4" class="px-4 py-8 text-center text-gray-500">No active mutes</td></tr>
            {/each}
          </tbody>
        </table>
      </div>
    </div>
  {/if}
</div>
