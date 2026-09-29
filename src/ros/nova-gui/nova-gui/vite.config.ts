import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";
import glsl from 'vite-plugin-glsl';

// https://vitejs.dev/config/
export default defineConfig({
  plugins: [
    react(),
    glsl(),
    {
      // When hosted on hydra it needs to use credentials to fetch the js and css files
      name: 'crossorigin',
      transformIndexHtml(html) {
        return html.replace(/crossorigin/g, 'crossorigin="use-credentials"');
      },
    },
  ],
  base: "./",
  build: {
    assetsDir: "assets",
  },
});
