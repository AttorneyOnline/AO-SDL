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

  // Detect music categories (lines that don't contain file extensions)
  function isCategory(item) {
    return tab === 'music' && !item.includes('.');
  }
</script>

<div class="space-y-4">
  <h2 class="text-lg font-semibold">Content</h2>

  <div class="flex gap-4 border-b border-(--color-border)">
    <button onclick={() => tab = 'areas'} class="pb-2 text-sm border-b-2 transition-colors {tab === 'areas' ? 'border-(--color-accent) text-(--color-accent)' : 'border-transparent text-(--color-text-muted)'}">
      Areas ({content.areas.length})
    </button>
    <button onclick={() => tab = 'characters'} class="pb-2 text-sm border-b-2 transition-colors {tab === 'characters' ? 'border-(--color-accent) text-(--color-accent)' : 'border-transparent text-(--color-text-muted)'}">
      Characters ({content.characters.length})
    </button>
    <button onclick={() => tab = 'music'} class="pb-2 text-sm border-b-2 transition-colors {tab === 'music' ? 'border-(--color-accent) text-(--color-accent)' : 'border-transparent text-(--color-text-muted)'}">
      Music ({content.music.length})
    </button>
  </div>

  {#if loading}
    <p class="text-(--color-text-muted) text-sm">Loading...</p>
  {:else}
    <div class="bg-(--color-surface-1) border border-(--color-border) overflow-hidden">
      <div class="max-h-[65vh] overflow-y-auto">
        {#each currentList() as item, i}
          {#if isCategory(item)}
            <div class="px-4 py-2 bg-(--color-surface-2) text-xs font-semibold uppercase tracking-wider text-(--color-text-secondary) border-b border-(--color-border) sticky top-0">
              {item}
            </div>
          {:else}
            <div class="px-4 py-1.5 flex items-center gap-3 text-sm hover:bg-(--color-surface-2)/50 border-b border-(--color-border)/30">
              <span class="text-[10px] text-(--color-text-muted) w-6 text-right tabular-nums">{i + 1}</span>
              <span>{item}</span>
            </div>
          {/if}
        {:else}
          <div class="px-4 py-8 text-center text-(--color-text-muted) text-sm">No items</div>
        {/each}
      </div>
    </div>
  {/if}
</div>
