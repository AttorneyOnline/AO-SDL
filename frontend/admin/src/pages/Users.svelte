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
  <h2 class="text-2xl font-bold">Users</h2>

  {#if loading}
    <p class="text-gray-500">Loading...</p>
  {:else}
    <div class="bg-gray-900 rounded-xl border border-gray-800 overflow-hidden">
      <div class="overflow-x-auto">
        <table class="w-full text-sm">
          <thead>
            <tr class="text-left text-xs text-gray-500 border-b border-gray-800">
              <th class="px-4 py-2">Username</th>
              <th class="px-4 py-2">Role</th>
            </tr>
          </thead>
          <tbody class="divide-y divide-gray-800/50">
            {#each users as u}
              <tr class="hover:bg-gray-800/30">
                <td class="px-4 py-2 font-medium">{u.username}</td>
                <td class="px-4 py-2">
                  <span class="text-xs px-1.5 py-0.5 rounded
                    {u.acl === 'SUPER' ? 'bg-purple-900/50 text-purple-300' :
                     u.acl === 'NONE' ? 'bg-gray-800 text-gray-500' :
                     'bg-blue-900/50 text-blue-300'}">
                    {u.acl}
                  </span>
                </td>
              </tr>
            {:else}
              <tr><td colspan="2" class="px-4 py-8 text-center text-gray-500">No users</td></tr>
            {/each}
          </tbody>
        </table>
      </div>
    </div>
  {/if}
</div>
