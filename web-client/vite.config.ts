import { defineConfig } from "vite";

export default defineConfig({
  base: "./",
  root: "./",
  build: {
    outDir: "dist",
    emptyOutDir: true,
    sourcemap: false,
    target: "esnext",
    rollupOptions: {
      output: {
        manualChunks: undefined,
      },
    },
  },
  server: {
    port: 3000,
    headers: {
      "Cross-Origin-Opener-Policy": "same-origin",
      "Cross-Origin-Embedder-Policy": "require-corp",
    },
  },
});
