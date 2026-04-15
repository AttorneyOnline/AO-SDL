import { vitePreprocess } from '@sveltejs/vite-plugin-svelte';

export default {
  preprocess: vitePreprocess(),
  // runes mode is opt-in per file (Svelte 5 auto-detects $state usage).
  // Forcing runes: true globally breaks Svelte 4 libraries like lucide-svelte.
};
