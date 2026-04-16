<script>
  import { get, patch } from '../lib/api.js';
  import { Save, Search, Plus, Trash2 } from 'lucide-svelte';

  let config = $state(null);
  let original = $state(null);
  let loading = $state(true);
  let saving = $state(false);
  let result = $state('');
  let resultType = $state('');
  let error = $state('');
  let search = $state('');

  let columns = $state([]);

  async function load() {
    loading = true;
    const res = await get('/admin/config');
    if (res.status === 200) {
      config = res.data;
      original = JSON.parse(JSON.stringify(res.data));
      columns = [];
    } else {
      error = res.data?.reason || 'Failed to load config';
    }
    loading = false;
  }

  $effect(() => { load(); });

  function objectAt(depth) {
    let obj = config;
    for (let i = 0; i < depth; i++) {
      if (!obj || typeof obj !== 'object') return null;
      obj = obj[columns[i]];
    }
    return obj;
  }

  function selectKey(depth, key) {
    columns = [...columns.slice(0, depth), key];
  }

  function isNested(val) {
    return val !== null && typeof val === 'object' && !Array.isArray(val);
  }

  function buildPatch(orig, current) {
    if (orig === current) return {};
    if (typeof current !== 'object' || current === null || Array.isArray(current)) {
      return JSON.stringify(current) !== JSON.stringify(orig) ? current : {};
    }
    const r = {};
    for (const key of Object.keys(current)) {
      if (isNested(current[key]) && isNested(orig?.[key])) {
        const nested = buildPatch(orig[key], current[key]);
        if (Object.keys(nested).length > 0) r[key] = nested;
      } else if (JSON.stringify(current[key]) !== JSON.stringify(orig?.[key])) {
        r[key] = current[key];
      }
    }
    return r;
  }

  async function saveAndApply() {
    error = ''; result = '';
    const diff = buildPatch(original, config);
    if (typeof diff === 'object' && Object.keys(diff).length === 0) {
      result = 'No changes.'; resultType = 'info'; return;
    }
    saving = true;
    const res = await patch('/admin/config', typeof diff === 'object' ? diff : config);
    saving = false;
    if (res.status === 200) {
      result = res.data?.reload_summary || 'Saved and applied.';
      resultType = 'ok';
      original = JSON.parse(JSON.stringify(config));
    } else {
      error = res.data?.reason || 'Failed';
    }
  }

  // Search: flatten all keys and filter
  function flattenKeys(obj, prefix = '') {
    const keys = [];
    if (!obj || typeof obj !== 'object') return keys;
    for (const [k, v] of Object.entries(obj)) {
      const path = prefix ? `${prefix}/${k}` : k;
      keys.push({ path, key: k, value: v });
      if (isNested(v)) keys.push(...flattenKeys(v, path));
    }
    return keys;
  }

  let searchResults = $derived.by(() => {
    if (!search || !config) return [];
    const q = search.toLowerCase();
    return flattenKeys(config).filter(e =>
      e.path.toLowerCase().includes(q) ||
      (typeof e.value !== 'object' && String(e.value).toLowerCase().includes(q))
    ).slice(0, 30);
  });

  function navigateToKey(path) {
    const parts = path.split('/');
    // Navigate column by column
    columns = [];
    let obj = config;
    for (const part of parts) {
      if (obj && isNested(obj) && part in obj) {
        columns = [...columns, part];
        obj = obj[part];
      } else break;
    }
    search = '';
  }

  // Array editing
  function addArrayItem(arr) {
    if (arr.length > 0) {
      const sample = arr[arr.length - 1];
      if (typeof sample === 'string') arr.push('');
      else if (typeof sample === 'number') arr.push(0);
      else if (typeof sample === 'boolean') arr.push(false);
      else arr.push('');
    } else {
      arr.push('');
    }
  }

  function removeArrayItem(arr, index) {
    arr.splice(index, 1);
  }
</script>

<div class="space-y-3">
  <div class="flex items-center justify-between flex-wrap gap-2">
    <h2 class="text-lg font-semibold">Configuration</h2>
    <div class="flex gap-px items-center">
      <!-- Search -->
      <div class="relative">
        <Search size={13} class="absolute left-2.5 top-1/2 -translate-y-1/2 text-(--color-text-muted)" strokeWidth={1.5} />
        <input
          type="text" bind:value={search} placeholder="Search keys..."
          class="pl-8 pr-3 py-1.5 text-sm bg-(--color-surface-2) border border-(--color-border) text-(--color-text-primary)
                 placeholder:text-(--color-text-muted) focus:outline-none focus:border-(--color-border-active) w-48"
        />
        {#if searchResults.length > 0}
          <div class="absolute top-full left-0 right-0 z-10 mt-px bg-(--color-surface-1) border border-(--color-border) max-h-60 overflow-y-auto">
            {#each searchResults as sr}
              <button onclick={() => navigateToKey(sr.path)}
                class="w-full px-3 py-1.5 text-left text-xs hover:bg-(--color-surface-2) flex justify-between gap-2">
                <span class="text-(--color-text-secondary) truncate">{sr.path}</span>
                {#if !isNested(sr.value)}
                  <span class="text-(--color-text-muted) font-mono truncate max-w-20">{String(sr.value)}</span>
                {/if}
              </button>
            {/each}
          </div>
        {/if}
      </div>
      <button onclick={saveAndApply} disabled={saving}
        class="flex items-center gap-1.5 px-3 py-1.5 text-sm bg-(--color-accent) text-(--color-surface-0) hover:opacity-80 disabled:opacity-30">
        <Save size={13} strokeWidth={1.5} /> {saving ? 'Saving...' : 'Save & Apply'}
      </button>
    </div>
  </div>

  {#if error}
    <div class="text-xs text-red-500 bg-red-500/10 border border-red-500/20 px-3 py-2">{error}</div>
  {/if}
  {#if result}
    <pre class="text-xs {resultType === 'ok' ? 'text-emerald-400 bg-emerald-400/10 border-emerald-400/20' : 'text-(--color-text-secondary) bg-(--color-surface-2) border-(--color-border)'} border px-3 py-2 whitespace-pre-wrap">{result}</pre>
  {/if}

  {#if loading}
    <p class="text-(--color-text-muted) text-sm">Loading...</p>
  {:else if config}
    <div class="flex border border-(--color-border) overflow-hidden" style="height: 65vh">
      <!-- Root column -->
      <div class="w-48 shrink-0 border-r border-(--color-border) overflow-y-auto bg-(--color-surface-1)">
        {#each Object.keys(config) as key}
          {@const val = config[key]}
          <button onclick={() => selectKey(0, key)}
            class="w-full px-3 py-1.5 text-left text-sm flex items-center justify-between hover:bg-(--color-surface-2)
                   {columns[0] === key ? 'bg-(--color-surface-2) text-(--color-accent) font-medium' : 'text-(--color-text-secondary)'}">
            <span class="truncate">{key}</span>
            {#if isNested(val) || Array.isArray(val)}
              <span class="text-[10px] text-(--color-text-muted)">&#x203A;</span>
            {/if}
          </button>
        {/each}
      </div>

      <!-- Nested columns -->
      {#each columns as selectedKey, depth}
        {@const obj = objectAt(depth)}
        {@const val = obj?.[selectedKey]}
        {#if isNested(val)}
          <div class="w-48 shrink-0 border-r border-(--color-border) overflow-y-auto bg-(--color-surface-1)">
            {#each Object.keys(val) as key}
              {@const childVal = val[key]}
              <button onclick={() => selectKey(depth + 1, key)}
                class="w-full px-3 py-1.5 text-left text-sm flex items-center justify-between hover:bg-(--color-surface-2)
                       {columns[depth + 1] === key ? 'bg-(--color-surface-2) text-(--color-accent) font-medium' : 'text-(--color-text-secondary)'}">
                <span class="truncate">{key}</span>
                {#if isNested(childVal) || Array.isArray(childVal)}
                  <span class="text-[10px] text-(--color-text-muted)">&#x203A;</span>
                {:else}
                  <span class="text-[10px] text-(--color-text-muted) font-mono truncate ml-2 max-w-16">{JSON.stringify(childVal)}</span>
                {/if}
              </button>
            {/each}
          </div>
        {/if}
      {/each}

      <!-- Value editor panel -->
      <div class="flex-1 overflow-y-auto bg-(--color-surface-0) p-4">
        {#if columns.length === 0}
          <p class="text-(--color-text-muted) text-sm">Select a key to edit</p>
        {:else}
          {@const parentObj = objectAt(columns.length - 1)}
          {@const key = columns[columns.length - 1]}
          {@const val = parentObj?.[key]}

          <h3 class="text-xs font-semibold uppercase tracking-wider text-(--color-text-muted) mb-3">{columns.join(' / ')}</h3>

          {#if Array.isArray(val)}
            <!-- Array editor -->
            <div class="space-y-1">
              {#each val as item, i}
                <div class="flex items-center gap-2">
                  <span class="text-[10px] text-(--color-text-muted) w-5 text-right tabular-nums">{i}</span>
                  {#if typeof item === 'boolean'}
                    <input type="checkbox" checked={item} onchange={(e) => val[i] = e.target.checked} class="accent-(--color-accent)" />
                  {:else if typeof item === 'number'}
                    <input type="number" value={item} onchange={(e) => val[i] = Number(e.target.value)} step="any"
                      class="flex-1 px-2 py-1 text-xs bg-(--color-surface-2) border border-(--color-border) text-(--color-text-primary) font-mono focus:outline-none focus:border-(--color-border-active)" />
                  {:else if typeof item === 'object'}
                    <textarea rows="2" value={JSON.stringify(item, null, 2)} onchange={(e) => { try { val[i] = JSON.parse(e.target.value); } catch {} }}
                      class="flex-1 px-2 py-1 text-xs bg-(--color-surface-2) border border-(--color-border) text-(--color-text-primary) font-mono focus:outline-none focus:border-(--color-border-active)"></textarea>
                  {:else}
                    <input type="text" value={item} oninput={(e) => val[i] = e.target.value}
                      class="flex-1 px-2 py-1 text-xs bg-(--color-surface-2) border border-(--color-border) text-(--color-text-primary) focus:outline-none focus:border-(--color-border-active)" />
                  {/if}
                  <button onclick={() => removeArrayItem(val, i)} class="p-1 text-(--color-text-muted) hover:text-red-400">
                    <Trash2 size={12} strokeWidth={1.5} />
                  </button>
                </div>
              {/each}
              <button onclick={() => addArrayItem(val)}
                class="flex items-center gap-1 mt-2 px-2 py-1 text-xs text-(--color-text-muted) hover:text-(--color-text-primary) border border-dashed border-(--color-border) hover:border-(--color-border-active)">
                <Plus size={12} strokeWidth={1.5} /> Add item
              </button>
            </div>
          {:else if isNested(val)}
            <!-- Nested object: show leaf values -->
            <div class="space-y-2">
              {#each Object.entries(val) as [k, v]}
                {#if !isNested(v) && !Array.isArray(v)}
                  <div class="flex items-center gap-3">
                    <span class="text-xs text-(--color-text-secondary) w-44 shrink-0 truncate" title={k}>{k}</span>
                    {#if typeof v === 'boolean'}
                      <input type="checkbox" checked={v} onchange={(e) => val[k] = e.target.checked} class="accent-(--color-accent)" />
                    {:else if typeof v === 'number'}
                      <input type="number" value={v} onchange={(e) => val[k] = Number(e.target.value)} step="any"
                        class="flex-1 px-2 py-1 text-xs bg-(--color-surface-2) border border-(--color-border) text-(--color-text-primary) font-mono focus:outline-none focus:border-(--color-border-active)" />
                    {:else}
                      <input type="text" value={v ?? ''} oninput={(e) => val[k] = e.target.value}
                        class="flex-1 px-2 py-1 text-xs bg-(--color-surface-2) border border-(--color-border) text-(--color-text-primary) focus:outline-none focus:border-(--color-border-active)" />
                    {/if}
                  </div>
                {/if}
              {/each}
            </div>
          {:else}
            <!-- Single value -->
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
