<script>
  import { get } from '../lib/api.js';

  let users = $state([]);
  let loading = $state(true);

  async function refresh() {
    const res = await get('/admin/users');
    if (res.status === 200) users = res.data?.users || [];
    loading = false;
  }

  $effect(() => { refresh(); });
</script>

<div class="space-y-4">
  <h2 class="text-lg font-semibold">Accounts</h2>

  {#if loading}
    <p class="text-(--color-text-muted) text-sm">Loading...</p>
  {:else}
    <div class="bg-(--color-surface-1) border border-(--color-border) overflow-hidden">
      <div class="overflow-x-auto">
        <table class="w-full text-sm">
          <thead>
            <tr class="text-left text-[10px] uppercase tracking-wider text-(--color-text-muted) border-b border-(--color-border)">
              <th class="px-4 py-2">Username</th>
              <th class="px-4 py-2">Role</th>
            </tr>
          </thead>
          <tbody class="divide-y divide-(--color-border)/50">
            {#each users as u}
              <tr class="hover:bg-(--color-surface-2)/50">
                <td class="px-4 py-1.5 font-medium">{u.username}</td>
                <td class="px-4 py-1.5">
                  <span class="text-[10px] px-1 py-px font-medium
                    {u.acl === 'SUPER' ? 'bg-violet-500/15 text-violet-400' :
                     u.acl === 'NONE' ? 'bg-(--color-surface-3) text-(--color-text-muted)' :
                     'bg-cyan-500/15 text-cyan-400'}">
                    {u.acl}
                  </span>
                </td>
              </tr>
            {:else}
              <tr><td colspan="2" class="px-4 py-8 text-center text-(--color-text-muted)">No accounts</td></tr>
            {/each}
          </tbody>
        </table>
      </div>
    </div>
  {/if}
</div>
