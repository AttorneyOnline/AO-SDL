<script>
  import { getSessionToken } from '../lib/api.js';
  import { MessageSquare } from 'lucide-svelte';

  let messages = $state([]);
  let connected = $state(false);
  let filter = $state('');
  let maxMessages = 500;

  function connect() {
    const token = getSessionToken();
    if (!token) return;
    fetchSSE(token);
  }

  async function fetchSSE(token) {
    try {
      const response = await fetch('/aonx/v1/events', {
        headers: { 'Authorization': `Bearer ${token}` },
      });

      if (!response.ok) { connected = false; return; }
      connected = true;
      const reader = response.body.getReader();
      const decoder = new TextDecoder();
      let buffer = '';

      while (true) {
        const { done, value } = await reader.read();
        if (done) break;
        buffer += decoder.decode(value, { stream: true });
        const lines = buffer.split('\n');
        buffer = lines.pop() || '';

        let currentEvent = '';
        let currentData = '';

        for (const line of lines) {
          if (line.startsWith('event: ')) currentEvent = line.slice(7);
          else if (line.startsWith('data: ')) currentData = line.slice(6);
          else if (line === '' && currentEvent && currentData) {
            handleEvent(currentEvent, currentData);
            currentEvent = '';
            currentData = '';
          }
        }
      }
    } catch (e) { console.error('SSE error:', e); }
    connected = false;
  }

  function handleEvent(event, dataStr) {
    if (event !== 'ic_message' && event !== 'ooc_message') return;
    try {
      const data = JSON.parse(dataStr);
      messages = [{
        type: event === 'ic_message' ? 'IC' : 'OOC',
        name: data.showname || data.name || data.character || '???',
        text: data.message || '',
        time: new Date().toLocaleTimeString('en-US', { hour12: false }),
        area: data.area || '',
      }, ...messages.slice(0, maxMessages - 1)];
    } catch {}
  }

  $effect(() => {
    connect();
  });

  let filtered = $derived(
    filter
      ? messages.filter(m =>
          m.text.toLowerCase().includes(filter.toLowerCase()) ||
          m.name.toLowerCase().includes(filter.toLowerCase()) ||
          m.area.toLowerCase().includes(filter.toLowerCase()))
      : messages
  );
</script>

<div class="space-y-4">
  <div class="flex items-center justify-between flex-wrap gap-2">
    <div class="flex items-center gap-2">
      <h2 class="text-lg font-semibold">Traffic</h2>
      <span class="w-1.5 h-1.5 {connected ? 'bg-emerald-500' : 'bg-red-500'}"></span>
    </div>
    <input
      type="text"
      bind:value={filter}
      placeholder="Filter..."
      class="px-3 py-1.5 text-sm bg-(--color-surface-2) border border-(--color-border) text-(--color-text-primary)
             placeholder:text-(--color-text-muted) focus:outline-none focus:border-(--color-border-active) w-56"
    />
  </div>

  <div class="bg-(--color-surface-1) border border-(--color-border) overflow-hidden">
    <div class="max-h-[75vh] overflow-y-auto font-mono text-xs leading-relaxed">
      {#each filtered as msg}
        <div class="px-3 py-1 flex gap-2 hover:bg-(--color-surface-2)/50 border-b border-(--color-border)/30">
          <span class="text-(--color-text-muted) shrink-0 w-16">{msg.time}</span>
          <span class="shrink-0 w-6 text-center font-bold {msg.type === 'IC' ? 'text-cyan-400' : 'text-emerald-400'}">
            {msg.type}
          </span>
          {#if msg.area}
            <span class="text-(--color-text-muted) shrink-0">[{msg.area}]</span>
          {/if}
          <span class="text-amber-400 shrink-0">{msg.name}</span>
          <span class="text-(--color-text-primary) break-all">{msg.text}</span>
        </div>
      {:else}
        <div class="px-4 py-12 text-center text-(--color-text-muted)">
          {connected ? 'Waiting for messages...' : 'Not connected'}
        </div>
      {/each}
    </div>
  </div>
</div>
