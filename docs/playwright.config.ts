import { defineConfig } from '@playwright/test';

export default defineConfig({
  testDir: './browser-tests',
  timeout: 60000,
  expect: { timeout: 15000 },
  workers: 1,
  retries: process.env.CI ? 1 : 0,
  reporter: [['list']],
  use: {
    baseURL: 'http://127.0.0.1:4318',
    browserName: 'chromium',
    channel: process.env.PLAYWRIGHT_CHANNEL,
    headless: Boolean(process.env.CI),
    ignoreHTTPSErrors: false,
    launchOptions: { chromiumSandbox: true },
    screenshot: 'only-on-failure',
    trace: 'off',
    video: 'off'
  },
  webServer: {
    command: 'python3 ../tools/serve_site.py dist --port 4318 --test-fixtures',
    url: 'http://127.0.0.1:4318/solar-system-simulator/',
    reuseExistingServer: false
  }
});
