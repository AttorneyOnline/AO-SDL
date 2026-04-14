<script>
  import { getSessionToken } from '../lib/api.js';

  let messages = $state([]);
  let connected = $state(false);
  let filter = $state('');
  let maxMessages = 500;
  /** @type {EventSource|null} */
  let eventSource = null;

  function connect() {
    const token = getSessionToken();
    if (!token) return;

    eventSource = new EventSource(`/aonx/v1/events`);

    // EventSource doesn't support custom headers, so we need to pass
    // the token as a query param or use a different approach.
    // For now, close and reconnect with fetch-based SSE if needed.
    // Actually, the SSE endpoint validates via Authorization header,
    // which EventSource doesn't support. We'll use fetch-based SSE.
    eventSource.close();
    eventSource = null;

    fetchSSE(token);
  }

  async function fetchSSE(token) {
    try {
      const response = await fetch('/aonx/v1/events', {
        headers: { 'Authorization': `Bearer ${token}` },
      });

      if (!response.ok) {
        connected = false;
        return;
      }

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
          if (line.startsWith('event: ')) {
            currentEvent = line.slice(7);
          } else if (line.startsWith('data: ')) {
            currentData = line.slice(6);
          } else if (line === '' && currentEvent && currentData) {
            handleEvent(currentEvent, currentData);
            currentEvent = '';
            currentData = '';
          }
        }
      }
    } catch (e) {
      console.error('SSE error:', e);
    }
    connected = false;
  }

  function handleEvent(event, dataStr) {
    if (event !== 'ic_message' && event !== 'ooc_message') return;

    try {
      const data = JSON.parse(dataStr);
      const msg = {
        type: event === 'ic_message' ? 'IC' : 'OOC',
        name: data.showname || data.name || data.character || '???',
        text: data.message || '',
        time: new Date().toLocaleTimeString(),
        area: data.area || '',
      };
      messages = [msg, ...messages.slice(0, maxMessages - 1)];
    } catch {}
  }

  $effect(() => {
    connect();
    return () => {
      if (eventSource) eventSource.close();
    };
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
    <h2 class="text-2xl font-bold">
      Live Traffic
      {#if connected}
        <span class="inline-block w-2 h-2 rounded-full bg-green-500 ml-2 animate-pulse"></span>
      {:else}
        <span class="inline-block w-2 h-2 rounded-full bg-red-500 ml-2"></span>
      {/if}
    </h2>
    <input
      type="text"
      bind:value={filter}
      placeholder="Filter by name, text, or area..."
      class="px-3 py-1.5 text-sm bg-gray-800 border border-gray-700 rounded-lg text-white placeholder-gray-500 w-64"
    />
  </div>

  <div class="bg-gray-900 rounded-xl border border-gray-800 overflow-hidden">
    <div class="max-h-[70vh] overflow-y-auto divide-y divide-gray-800/50 font-mono text-xs">
      {#each filtered as msg}
        <div class="px-3 py-1.5 flex gap-2 hover:bg-gray-800/30">
          <span class="text-gray-600 shrink-0">{msg.time}</span>
          <span class="shrink-0 w-8 text-center font-bold {msg.type === 'IC' ? 'text-blue-400' : 'text-green-400'}">
            {msg.type}
          </span>
          {#if msg.area}
            <span class="text-gray-500 shrink-0">[{msg.area}]</span>
          {/if}
          <span class="text-yellow-300 shrink-0">{msg.name}:</span>
          <span class="text-gray-200 break-all">{msg.text}</span>
        </div>
      {:else}
        <div class="px-4 py-8 text-center text-gray-500">
          {connected ? 'Waiting for messages...' : 'Not connected to event stream'}
        </div>
      {/each}
    </div>
  </div>
</div>
