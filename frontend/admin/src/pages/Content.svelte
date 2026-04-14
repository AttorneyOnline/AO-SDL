<script>
  import { get } from '../lib/api.js';

  let tab = $state('areas');
  let content = $state({ characters: [], music: [], areas: [] });
  let loading = $state(true);

  async function refresh() {
    const res = await get('/admin/content');
    if (res.status === 200) content = res.data;
    loading = false;
  }

  $effect(() => { refresh(); });

  function currentList() {
    if (tab === 'areas') return content.areas;
    if (tab === 'characters') return content.characters;
    if (tab === 'music') return content.music;
    return [];
  }
</script>

<div class="space-y-4">
  <h2 class="text-2xl font-bold">Content</h2>

  <div class="flex gap-1 border-b border-gray-800">
    <button onclick={() => tab = 'areas'} class="px-4 py-2 text-sm {tab === 'areas' ? 'text-white border-b-2 border-blue-500' : 'text-gray-400'}">
      Areas ({content.areas.length})
    </button>
    <button onclick={() => tab = 'characters'} class="px-4 py-2 text-sm {tab === 'characters' ? 'text-white border-b-2 border-blue-500' : 'text-gray-400'}">
      Characters ({content.characters.length})
    </button>
    <button onclick={() => tab = 'music'} class="px-4 py-2 text-sm {tab === 'music' ? 'text-white border-b-2 border-blue-500' : 'text-gray-400'}">
      Music ({content.music.length})
    </button>
  </div>

  {#if loading}
    <p class="text-gray-500">Loading...</p>
  {:else}
    <div class="bg-gray-900 rounded-xl border border-gray-800 overflow-hidden">
      <div class="max-h-[60vh] overflow-y-auto divide-y divide-gray-800/50">
        {#each currentList() as item, i}
          <div class="px-4 py-2 flex items-center gap-3 text-sm hover:bg-gray-800/30">
            <span class="text-xs text-gray-600 w-8 text-right tabular-nums">{i + 1}</span>
            <span class="text-gray-200">{item}</span>
          </div>
        {:else}
          <div class="px-4 py-8 text-center text-gray-500">No items</div>
        {/each}
      </div>
    </div>
  {/if}
</div>
