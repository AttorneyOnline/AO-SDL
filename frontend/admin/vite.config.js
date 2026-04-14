import { defineConfig } from 'vite';
import { svelte } from '@sveltejs/vite-plugin-svelte';
import tailwindcss from '@tailwindcss/vite';
import path from 'path';

export default defineConfig({
  plugins: [tailwindcss(), svelte()],
  base: '/admin/',
  build: {
    outDir: path.resolve(__dirname, '../../assets/admin'),
    emptyOutDir: true,
    assetsDir: 'assets',
  },
  server: {
    proxy: {
      '/aonx': 'http://localhost:8080',
      '/metrics': 'http://localhost:8080',
    },
  },
});
