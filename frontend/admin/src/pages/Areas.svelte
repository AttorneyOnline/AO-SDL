<script>
  import { get } from '../lib/api.js';

  let areas = $state([]);
  let loading = $state(true);
  let selected = $state(null);

  async function refresh() {
    const res = await get('/areas');
    if (res.status === 200) areas = res.data?.areas || [];
    loading = false;
  }

  $effect(() => {
    refresh();
    const interval = setInterval(refresh, 10000);
    return () => clearInterval(interval);
  });

  async function loadDetail(areaId) {
    const res = await get(`/areas/${areaId}`);
    if (res.status === 200) selected = res.data;
  }
</script>

<div class="space-y-4">
  <h2 class="text-2xl font-bold">Areas</h2>

  {#if loading}
    <p class="text-gray-500">Loading...</p>
  {:else}
    <div class="grid gap-3 sm:grid-cols-2 lg:grid-cols-3">
      {#each areas as area}
        <button
          onclick={() => loadDetail(area.id)}
          class="bg-gray-900 rounded-xl border border-gray-800 p-4 text-left hover:border-gray-600 transition-colors
                 {selected?.area?.id === area.id ? 'border-blue-500' : ''}"
        >
          <div class="flex items-center justify-between mb-2">
            <span class="font-semibold text-sm">{area.name}</span>
            <span class="text-xs text-gray-500">{area.players ?? 0} players</span>
          </div>
          <div class="flex gap-2 text-xs">
            {#if area.status && area.status !== 'IDLE'}
              <span class="px-1.5 py-0.5 rounded bg-blue-900/50 text-blue-300">{area.status}</span>
            {:else}
              <span class="px-1.5 py-0.5 rounded bg-gray-800 text-gray-500">IDLE</span>
            {/if}
            {#if area.locked && area.locked !== 'FREE'}
              <span class="px-1.5 py-0.5 rounded bg-yellow-900/50 text-yellow-300">{area.locked}</span>
            {/if}
            {#if area.cm}
              <span class="px-1.5 py-0.5 rounded bg-green-900/50 text-green-300">CM: {area.cm}</span>
            {/if}
          </div>
        </button>
      {/each}
    </div>

    {#if selected}
      <div class="bg-gray-900 rounded-xl border border-gray-800 p-4 space-y-3">
        <h3 class="text-sm font-semibold">{selected.area?.name || 'Area Detail'}</h3>
        <div class="grid grid-cols-2 sm:grid-cols-3 gap-3 text-xs">
          {#if selected.background}
            <div><span class="text-gray-500">Background</span><br>{selected.background.name}</div>
          {/if}
          {#if selected.music}
            <div><span class="text-gray-500">Music</span><br>{selected.music.name || 'None'}</div>
          {/if}
          {#if selected.hp}
            <div><span class="text-gray-500">HP</span><br>Def: {selected.hp.defense} / Pro: {selected.hp.prosecution}</div>
          {/if}
        </div>
      </div>
    {/if}
  {/if}
</div>
