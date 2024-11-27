import { defineWorkspace } from 'vitest/config'

export default defineWorkspace([
  // If you want to keep running your existing tests in Node.js, uncomment the next line.
  // 'vite.config.ts',
  {
    extends: 'vite.config.ts',
    test: {
      browser: {
        enabled: true,
        name: 'chrome',
        provider: 'webdriverio',
        // https://webdriver.io
        providerOptions: {
          capabilities: {
            browserName: 'chrome',
            'goog:chromeOptions': {
              binary: '/etc/profiles/per-user/nova/bin/chromium',
               args: ["--ozone-platform=wayland"]
            },
            'wdio:chromedriverOptions': {
              binary: '/run/current-system/sw/bin/chromedriver'
            },
          },
        },
      },
    },
  },
])
