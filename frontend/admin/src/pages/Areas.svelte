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
  <h2 class="text-lg font-semibold">Areas</h2>

  {#if loading}
    <p class="text-(--color-text-muted) text-sm">Loading...</p>
  {:else}
    <div class="grid gap-px bg-(--color-border) sm:grid-cols-2 lg:grid-cols-3">
      {#each areas as area}
        <button
          onclick={() => loadDetail(area.id)}
          class="bg-(--color-surface-1) p-3 text-left hover:bg-(--color-surface-2) transition-colors
                 {selected?.area?.id === area.id ? 'ring-1 ring-(--color-accent) ring-inset' : ''}"
        >
          <div class="flex items-center justify-between mb-1">
            <span class="font-medium text-sm">{area.name}</span>
            <span class="text-xs text-(--color-text-muted) tabular-nums">{area.players ?? 0}</span>
          </div>
          <div class="flex gap-1 text-[10px]">
            {#if area.status && area.status !== 'IDLE'}
              <span class="px-1 py-px bg-cyan-500/15 text-cyan-400 font-medium">{area.status}</span>
            {:else}
              <span class="px-1 py-px bg-(--color-surface-3) text-(--color-text-muted)">IDLE</span>
            {/if}
            {#if area.locked && area.locked !== 'FREE'}
              <span class="px-1 py-px bg-amber-500/15 text-amber-400 font-medium">{area.locked}</span>
            {/if}
          </div>
        </button>
      {/each}
    </div>

    {#if selected}
      <div class="bg-(--color-surface-1) border border-(--color-border) p-4 space-y-3">
        <h3 class="text-sm font-semibold">{selected.area?.name}</h3>
        <div class="grid grid-cols-2 sm:grid-cols-3 gap-3 text-xs">
          {#if selected.background}
            <div><span class="text-(--color-text-muted) text-[10px] uppercase">Background</span><div class="mt-0.5">{selected.background.name}</div></div>
          {/if}
          {#if selected.music}
            <div><span class="text-(--color-text-muted) text-[10px] uppercase">Music</span><div class="mt-0.5">{selected.music.name || 'None'}</div></div>
          {/if}
          {#if selected.hp}
            <div><span class="text-(--color-text-muted) text-[10px] uppercase">HP</span><div class="mt-0.5">Def {selected.hp.defense} / Pro {selected.hp.prosecution}</div></div>
          {/if}
        </div>
      </div>
    {/if}
  {/if}
</div>
