<script>
  import { get, api } from '../lib/api.js';
  import { Plus, Trash2, Save, GripVertical } from 'lucide-svelte';

  let tab = $state('areas');
  let content = $state({ characters: [], music: [], areas: [] });
  let original = $state({ characters: [], music: [], areas: [] });
  let loading = $state(true);
  let saving = $state(false);
  let result = $state('');
  let newItem = $state('');

  async function refresh() {
    const res = await get('/admin/content');
    if (res.status === 200) {
      content = res.data;
      original = JSON.parse(JSON.stringify(res.data));
    }
    loading = false;
  }

  $effect(() => { refresh(); });

  function currentList() {
    if (tab === 'areas') return content.areas;
    if (tab === 'characters') return content.characters;
    if (tab === 'music') return content.music;
    return [];
  }

  function setList(list) {
    if (tab === 'areas') content.areas = list;
    else if (tab === 'characters') content.characters = list;
    else if (tab === 'music') content.music = list;
  }

  function isCategory(item) {
    return tab === 'music' && !item.includes('.');
  }

  function hasChanges() {
    return JSON.stringify(content) !== JSON.stringify(original);
  }

  function addItem() {
    if (!newItem.trim()) return;
    const list = [...currentList(), newItem.trim()];
    setList(list);
    newItem = '';
  }

  function removeItem(index) {
    const list = [...currentList()];
    list.splice(index, 1);
    setList(list);
  }

  function moveItem(index, direction) {
    const list = [...currentList()];
    const target = index + direction;
    if (target < 0 || target >= list.length) return;
    [list[index], list[target]] = [list[target], list[index]];
    setList(list);
  }

  async function save() {
    if (!hasChanges()) { result = 'No changes.'; return; }
    saving = true;
    result = '';
    const res = await api('PUT', '/admin/content', content);
    saving = false;
    if (res.status === 200) {
      result = res.data?.reload_summary || 'Content saved and applied.';
      original = JSON.parse(JSON.stringify(content));
    } else {
      result = res.data?.reason || 'Failed to save';
    }
  }
</script>

<div class="space-y-3">
  <div class="flex items-center justify-between flex-wrap gap-2">
    <h2 class="text-lg font-semibold">Content</h2>
    <button onclick={save} disabled={saving || !hasChanges()}
      class="flex items-center gap-1.5 px-3 py-1.5 text-sm bg-(--color-accent) text-(--color-surface-0)
             hover:opacity-80 disabled:opacity-30">
      <Save size={13} strokeWidth={1.5} /> {saving ? 'Saving...' : 'Save & Apply'}
    </button>
  </div>

  {#if result}
    <pre class="text-xs text-emerald-400 bg-emerald-400/10 border border-emerald-400/20 px-3 py-2 whitespace-pre-wrap">{result}</pre>
  {/if}

  <div class="flex gap-4 border-b border-(--color-border)">
    {#each [['areas', 'Areas'], ['characters', 'Characters'], ['music', 'Music']] as [key, label]}
      <button onclick={() => { tab = key; newItem = ''; }}
        class="pb-2 text-sm border-b-2 transition-colors {tab === key ? 'border-(--color-accent) text-(--color-accent)' : 'border-transparent text-(--color-text-muted)'}">
        {label} ({content[key]?.length ?? 0})
      </button>
    {/each}
  </div>

  {#if loading}
    <p class="text-(--color-text-muted) text-sm">Loading...</p>
  {:else}
    <!-- Add new item -->
    <div class="flex gap-px">
      <input type="text" bind:value={newItem} placeholder="Add {tab === 'music' ? 'track or category' : tab.slice(0, -1)}..."
        onkeydown={(e) => e.key === 'Enter' && addItem()}
        class="flex-1 px-3 py-1.5 text-sm bg-(--color-surface-2) border border-(--color-border) text-(--color-text-primary)
               placeholder:text-(--color-text-muted) focus:outline-none focus:border-(--color-border-active)" />
      <button onclick={addItem} class="px-3 py-1.5 text-sm bg-(--color-surface-3) border border-(--color-border) text-(--color-text-secondary) hover:text-(--color-text-primary)">
        <Plus size={14} strokeWidth={1.5} />
      </button>
    </div>

    <!-- Content list -->
    <div class="bg-(--color-surface-1) border border-(--color-border) overflow-hidden">
      <div class="max-h-[60vh] overflow-y-auto">
        {#each currentList() as item, i}
          {#if isCategory(item)}
            <div class="px-4 py-2 bg-(--color-surface-2) text-xs font-semibold uppercase tracking-wider text-(--color-text-secondary) border-b border-(--color-border) sticky top-0 flex items-center justify-between">
              <span>{item}</span>
              <button onclick={() => removeItem(i)} class="p-0.5 text-(--color-text-muted) hover:text-red-400">
                <Trash2 size={11} strokeWidth={1.5} />
              </button>
            </div>
          {:else}
            <div class="px-2 py-1 flex items-center gap-1 text-sm hover:bg-(--color-surface-2)/50 border-b border-(--color-border)/30 group">
              <span class="text-[10px] text-(--color-text-muted) w-6 text-right tabular-nums shrink-0">{i + 1}</span>
              <div class="flex flex-col shrink-0 opacity-0 group-hover:opacity-100 transition-opacity">
                <button onclick={() => moveItem(i, -1)} class="text-[8px] text-(--color-text-muted) hover:text-(--color-text-primary) leading-none">&blacktriangle;</button>
                <button onclick={() => moveItem(i, 1)} class="text-[8px] text-(--color-text-muted) hover:text-(--color-text-primary) leading-none">&blacktriangledown;</button>
              </div>
              <span class="flex-1 truncate">{item}</span>
              <button onclick={() => removeItem(i)} class="p-0.5 text-(--color-text-muted) hover:text-red-400 opacity-0 group-hover:opacity-100 transition-opacity">
                <Trash2 size={12} strokeWidth={1.5} />
              </button>
            </div>
          {/if}
        {:else}
          <div class="px-4 py-8 text-center text-(--color-text-muted) text-sm">No items</div>
        {/each}
      </div>
    </div>
  {/if}
</div>
