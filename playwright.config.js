import { defineConfig } from "@playwright/test";
import { existsSync } from "node:fs";
export default defineConfig({
  testDir: "tests/browser",
  workers: 1,
  timeout: 30000,
  use: {
    baseURL: "http://127.0.0.1:8765",
    headless: true,
    launchOptions:
      process.env.CHROMIUM_PATH || existsSync("/usr/bin/chromium")
        ? { executablePath: process.env.CHROMIUM_PATH || "/usr/bin/chromium" }
        : {},
  },
  webServer: {
    command: "python3 -m http.server 8765 --bind 127.0.0.1 --directory web",
    url: "http://127.0.0.1:8765",
    reuseExistingServer: false,
  },
});
