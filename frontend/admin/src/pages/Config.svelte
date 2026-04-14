<script>
  import { get, patch } from '../lib/api.js';

  let config = $state(null);
  let original = $state(null);
  let loading = $state(true);
  let saving = $state(false);
  let reloadSummary = $state('');
  let error = $state('');

  async function load() {
    loading = true;
    const res = await get('/admin/config');
    if (res.status === 200) {
      config = res.data;
      original = JSON.parse(JSON.stringify(res.data));
    } else {
      error = res.data?.reason || 'Failed to load config';
    }
    loading = false;
  }

  $effect(() => { load(); });

  /** Build a merge patch from original → config (only changed keys). */
  function buildPatch(orig, current) {
    const result = {};
    for (const key of Object.keys(current)) {
      if (typeof current[key] === 'object' && current[key] !== null && !Array.isArray(current[key]) &&
          typeof orig[key] === 'object' && orig[key] !== null) {
        const nested = buildPatch(orig[key], current[key]);
        if (Object.keys(nested).length > 0) result[key] = nested;
      } else if (JSON.stringify(current[key]) !== JSON.stringify(orig[key])) {
        result[key] = current[key];
      }
    }
    return result;
  }

  async function save() {
    error = '';
    reloadSummary = '';
    const diff = buildPatch(original, config);
    if (Object.keys(diff).length === 0) {
      reloadSummary = 'No changes to save.';
      return;
    }
    saving = true;
    const res = await patch('/admin/config', diff);
    saving = false;
    if (res.status === 200) {
      reloadSummary = res.data?.reload_summary || 'Config saved and reloaded.';
      original = JSON.parse(JSON.stringify(config));
    } else {
      error = res.data?.reason || 'Failed to save config';
    }
  }

  async function reload() {
    reloadSummary = '';
    saving = true;
    const res = await patch('/admin/config', {});
    saving = false;
    if (res.status === 200) {
      reloadSummary = res.data?.reload_summary || 'Server reloaded.';
    } else {
      error = res.data?.reason || 'Reload failed';
    }
  }
</script>

<div class="space-y-4">
  <div class="flex items-center justify-between flex-wrap gap-2">
    <h2 class="text-2xl font-bold">Configuration</h2>
    <div class="flex gap-2">
      <button
        onclick={reload}
        disabled={saving}
        class="px-3 py-1.5 text-sm bg-gray-700 hover:bg-gray-600 disabled:opacity-50 rounded-lg transition-colors"
      >
        Reload Server
      </button>
      <button
        onclick={save}
        disabled={saving}
        class="px-3 py-1.5 text-sm bg-blue-600 hover:bg-blue-500 disabled:opacity-50 rounded-lg transition-colors"
      >
        {saving ? 'Saving...' : 'Save Changes'}
      </button>
    </div>
  </div>

  {#if error}
    <div class="text-sm text-red-400 bg-red-400/10 rounded-lg px-3 py-2">{error}</div>
  {/if}

  {#if reloadSummary}
    <pre class="text-xs text-green-400 bg-green-400/10 rounded-lg px-3 py-2 whitespace-pre-wrap">{reloadSummary}</pre>
  {/if}

  {#if loading}
    <p class="text-gray-500">Loading configuration...</p>
  {:else if config}
    <div class="space-y-3">
      {#each Object.entries(config) as [section, value]}
        <div class="bg-gray-900 rounded-xl border border-gray-800 overflow-hidden">
          <details>
            <summary class="px-4 py-3 cursor-pointer hover:bg-gray-800/50 text-sm font-medium flex items-center justify-between">
              <span>{section}</span>
              <span class="text-xs text-gray-500">
                {typeof value === 'object' ? Object.keys(value).length + ' keys' : typeof value}
              </span>
            </summary>
            <div class="px-4 py-3 border-t border-gray-800 bg-gray-950/50">
              {#if typeof value === 'object' && value !== null && !Array.isArray(value)}
                {@render objectEditor(config, section, value)}
              {:else}
                {@render fieldEditor(config, section, value)}
              {/if}
            </div>
          </details>
        </div>
      {/each}
    </div>
  {/if}
</div>

{#snippet objectEditor(parent, key, obj)}
  <div class="space-y-2">
    {#each Object.entries(obj) as [k, v]}
      {#if typeof v === 'object' && v !== null && !Array.isArray(v)}
        <details class="ml-2 border-l border-gray-800 pl-3">
          <summary class="text-xs text-gray-400 cursor-pointer hover:text-gray-200">{k}</summary>
          <div class="mt-2">
            {@render objectEditor(obj, k, v)}
          </div>
        </details>
      {:else}
        {@render fieldEditor(obj, k, v)}
      {/if}
    {/each}
  </div>
{/snippet}

{#snippet fieldEditor(parent, key, value)}
  <div class="flex items-center gap-2 py-1">
    <label class="text-xs text-gray-400 w-48 shrink-0 truncate" title={key}>{key}</label>
    {#if typeof value === 'boolean'}
      <input
        type="checkbox"
        checked={value}
        onchange={(e) => parent[key] = e.target.checked}
        class="rounded bg-gray-800 border-gray-600"
      />
    {:else if typeof value === 'number'}
      <input
        type="number"
        value={value}
        onchange={(e) => parent[key] = Number(e.target.value)}
        step="any"
        class="flex-1 px-2 py-1 text-xs bg-gray-800 border border-gray-700 rounded text-white"
      />
    {:else if Array.isArray(value)}
      <textarea
        class="flex-1 px-2 py-1 text-xs bg-gray-800 border border-gray-700 rounded text-white font-mono"
        rows="2"
        value={JSON.stringify(value, null, 2)}
        onchange={(e) => { try { parent[key] = JSON.parse(e.target.value); } catch {} }}
      ></textarea>
    {:else}
      <input
        type="text"
        value={value ?? ''}
        oninput={(e) => parent[key] = e.target.value}
        class="flex-1 px-2 py-1 text-xs bg-gray-800 border border-gray-700 rounded text-white"
      />
    {/if}
  </div>
{/snippet}
