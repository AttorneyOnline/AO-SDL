<script>
  import { get, patch } from '../lib/api.js';
  import { RefreshCw, Save } from 'lucide-svelte';

  let config = $state(null);
  let original = $state(null);
  let loading = $state(true);
  let saving = $state(false);
  let reloadSummary = $state('');
  let error = $state('');

  // Column navigation state: array of selected keys at each depth
  let columns = $state([]);

  async function load() {
    loading = true;
    const res = await get('/admin/config');
    if (res.status === 200) {
      config = res.data;
      original = JSON.parse(JSON.stringify(res.data));
      columns = []; // reset navigation
    } else {
      error = res.data?.reason || 'Failed to load config';
    }
    loading = false;
  }

  $effect(() => { load(); });

  // Get the object at a given column depth
  function objectAt(depth) {
    let obj = config;
    for (let i = 0; i < depth; i++) {
      if (!obj || typeof obj !== 'object') return null;
      obj = obj[columns[i]];
    }
    return obj;
  }

  // Select a key at a given depth
  function selectKey(depth, key) {
    columns = [...columns.slice(0, depth), key];
  }

  // Get the leaf value for editing (the object after all column selections)
  let leafObj = $derived.by(() => {
    if (!config || columns.length === 0) return null;
    return objectAt(columns.length);
  });

  // Check if a value is a nested object (not array, not null)
  function isNested(val) {
    return val !== null && typeof val === 'object' && !Array.isArray(val);
  }

  function buildPatch(orig, current) {
    const result = {};
    for (const key of Object.keys(current)) {
      if (isNested(current[key]) && isNested(orig?.[key])) {
        const nested = buildPatch(orig[key], current[key]);
        if (Object.keys(nested).length > 0) result[key] = nested;
      } else if (JSON.stringify(current[key]) !== JSON.stringify(orig?.[key])) {
        result[key] = current[key];
      }
    }
    return result;
  }

  async function save() {
    error = ''; reloadSummary = '';
    const diff = buildPatch(original, config);
    if (Object.keys(diff).length === 0) { reloadSummary = 'No changes.'; return; }
    saving = true;
    const res = await patch('/admin/config', diff);
    saving = false;
    if (res.status === 200) {
      reloadSummary = res.data?.reload_summary || 'Saved.';
      original = JSON.parse(JSON.stringify(config));
    } else { error = res.data?.reason || 'Failed'; }
  }

  async function reload() {
    reloadSummary = ''; saving = true;
    const res = await patch('/admin/config', {});
    saving = false;
    if (res.status === 200) reloadSummary = res.data?.reload_summary || 'Reloaded.';
    else error = res.data?.reason || 'Failed';
  }

  function setLeafValue(parent, key, value, type) {
    if (type === 'boolean') parent[key] = value;
    else if (type === 'number') parent[key] = Number(value);
    else parent[key] = value;
  }
</script>

<div class="space-y-4">
  <div class="flex items-center justify-between flex-wrap gap-2">
    <h2 class="text-lg font-semibold">Configuration</h2>
    <div class="flex gap-px">
      <button onclick={reload} disabled={saving} class="flex items-center gap-1.5 px-3 py-1.5 text-sm bg-(--color-surface-3) border border-(--color-border) text-(--color-text-secondary) hover:text-(--color-text-primary) disabled:opacity-30">
        <RefreshCw size={13} strokeWidth={1.5} /> Reload
      </button>
      <button onclick={save} disabled={saving} class="flex items-center gap-1.5 px-3 py-1.5 text-sm bg-(--color-accent) text-(--color-surface-0) hover:opacity-80 disabled:opacity-30">
        <Save size={13} strokeWidth={1.5} /> {saving ? 'Saving...' : 'Save'}
      </button>
    </div>
  </div>

  {#if error}
    <div class="text-xs text-red-500 bg-red-500/10 border border-red-500/20 px-3 py-2">{error}</div>
  {/if}
  {#if reloadSummary}
    <pre class="text-xs text-emerald-400 bg-emerald-400/10 border border-emerald-400/20 px-3 py-2 whitespace-pre-wrap">{reloadSummary}</pre>
  {/if}

  {#if loading}
    <p class="text-(--color-text-muted) text-sm">Loading...</p>
  {:else if config}
    <!-- Finder column view -->
    <div class="flex border border-(--color-border) overflow-hidden" style="height: 65vh">
      <!-- Root column -->
      <div class="w-48 shrink-0 border-r border-(--color-border) overflow-y-auto bg-(--color-surface-1)">
        {#each Object.keys(config) as key}
          {@const val = config[key]}
          <button
            onclick={() => selectKey(0, key)}
            class="w-full px-3 py-1.5 text-left text-sm flex items-center justify-between hover:bg-(--color-surface-2)
                   {columns[0] === key ? 'bg-(--color-surface-2) text-(--color-accent) font-medium' : 'text-(--color-text-secondary)'}"
          >
            <span class="truncate">{key}</span>
            {#if isNested(val)}
              <span class="text-[10px] text-(--color-text-muted)">&#x203A;</span>
            {/if}
          </button>
        {/each}
      </div>

      <!-- Subsequent columns for nested objects -->
      {#each columns as selectedKey, depth}
        {@const obj = objectAt(depth)}
        {@const val = obj?.[selectedKey]}
        {#if isNested(val)}
          <div class="w-48 shrink-0 border-r border-(--color-border) overflow-y-auto bg-(--color-surface-1)">
            {#each Object.keys(val) as key}
              {@const childVal = val[key]}
              <button
                onclick={() => selectKey(depth + 1, key)}
                class="w-full px-3 py-1.5 text-left text-sm flex items-center justify-between hover:bg-(--color-surface-2)
                       {columns[depth + 1] === key ? 'bg-(--color-surface-2) text-(--color-accent) font-medium' : 'text-(--color-text-secondary)'}"
              >
                <span class="truncate">{key}</span>
                {#if isNested(childVal)}
                  <span class="text-[10px] text-(--color-text-muted)">&#x203A;</span>
                {:else}
                  <span class="text-[10px] text-(--color-text-muted) font-mono truncate ml-2 max-w-16">
                    {typeof childVal === 'string' ? childVal : JSON.stringify(childVal)}
                  </span>
                {/if}
              </button>
            {/each}
          </div>
        {/if}
      {/each}

      <!-- Value editor panel (rightmost) -->
      <div class="flex-1 overflow-y-auto bg-(--color-surface-0) p-4">
        {#if columns.length === 0}
          <p class="text-(--color-text-muted) text-sm">Select a key to edit</p>
        {:else}
          {@const parentDepth = columns.length - 1}
          {@const parentObj = objectAt(parentDepth)}
          {@const key = columns[parentDepth]}
          {@const val = parentObj?.[key]}
          {#if isNested(val)}
            <!-- Show all leaf values in this section -->
            <h3 class="text-xs font-semibold uppercase tracking-wider text-(--color-text-muted) mb-3">{columns.join(' / ')}</h3>
            <div class="space-y-2">
              {#each Object.entries(val) as [k, v]}
                {#if !isNested(v)}
                  <div class="flex items-center gap-3">
                    <span class="text-xs text-(--color-text-secondary) w-44 shrink-0 truncate" title={k}>{k}</span>
                    {#if typeof v === 'boolean'}
                      <input type="checkbox" checked={v} onchange={(e) => val[k] = e.target.checked} class="accent-(--color-accent)" />
                    {:else if typeof v === 'number'}
                      <input type="number" value={v} onchange={(e) => val[k] = Number(e.target.value)} step="any"
                        class="flex-1 px-2 py-1 text-xs bg-(--color-surface-2) border border-(--color-border) text-(--color-text-primary) font-mono focus:outline-none focus:border-(--color-border-active)" />
                    {:else if Array.isArray(v)}
                      <textarea rows="2" value={JSON.stringify(v, null, 2)} onchange={(e) => { try { val[k] = JSON.parse(e.target.value); } catch {} }}
                        class="flex-1 px-2 py-1 text-xs bg-(--color-surface-2) border border-(--color-border) text-(--color-text-primary) font-mono focus:outline-none focus:border-(--color-border-active)" ></textarea>
                    {:else}
                      <input type="text" value={v ?? ''} oninput={(e) => val[k] = e.target.value}
                        class="flex-1 px-2 py-1 text-xs bg-(--color-surface-2) border border-(--color-border) text-(--color-text-primary) focus:outline-none focus:border-(--color-border-active)" />
                    {/if}
                  </div>
                {/if}
              {/each}
            </div>
          {:else}
            <!-- Single value editor -->
            <h3 class="text-xs font-semibold uppercase tracking-wider text-(--color-text-muted) mb-3">{columns.join(' / ')}</h3>
            <div class="flex items-center gap-3">
              {#if typeof val === 'boolean'}
                <input type="checkbox" checked={val} onchange={(e) => parentObj[key] = e.target.checked} class="accent-(--color-accent)" />
                <span class="text-sm">{val ? 'true' : 'false'}</span>
              {:else if typeof val === 'number'}
                <input type="number" value={val} onchange={(e) => parentObj[key] = Number(e.target.value)} step="any"
                  class="w-48 px-2 py-1 text-sm bg-(--color-surface-2) border border-(--color-border) text-(--color-text-primary) font-mono focus:outline-none focus:border-(--color-border-active)" />
              {:else}
                <input type="text" value={val ?? ''} oninput={(e) => parentObj[key] = e.target.value}
                  class="flex-1 px-2 py-1 text-sm bg-(--color-surface-2) border border-(--color-border) text-(--color-text-primary) focus:outline-none focus:border-(--color-border-active)" />
              {/if}
            </div>
          {/if}
        {/if}
      </div>
    </div>
  {/if}
</div>
