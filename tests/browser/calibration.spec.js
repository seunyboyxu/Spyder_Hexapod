import { test, expect } from "@playwright/test";
test.beforeEach(async ({ page }) => {
  page.on("dialog", (dialog) => dialog.accept());
  await page.goto("/");
});
test("USB-only servo supply warning is visible before arming", async ({ page }) => {
  await expect(page.locator("aside").getByText("USB-only servo power is unsuitable.", { exact: true })).toBeVisible();
});
test("acknowledged initial calibration becomes the explicit arming pulse", async ({ page }) => {
  await page.locator("#demo").click();
  await expect(page.locator("#connection")).toContainText("DEMO");
  const row = page.locator('[data-joint="0"]');
  await row.locator("summary").click();
  await row.getByLabel("Initial pulse (µs)").fill("1450");
  await row.getByRole("button", { name: "Apply calibration" }).click();
  await expect(row.locator(".pulse-input")).toHaveValue("1450");
  await page.locator("#supported").check();
  await page.locator("#resume").click();
  await row.locator(".arm").click();
  await expect(row.locator(".commanded")).toHaveText("1450");
});

test("Release bypasses a busy action and its acknowledgement remains visible", async ({ page }) => {
  await page.evaluate(async () => {
    const { DemoTransport } = await import("/demo.js");
    const write = DemoTransport.prototype.write;
    DemoTransport.prototype.write = function (line) {
      (window.sentCommands ??= []).push(line);
      if (line.includes(" ARM ")) window.delayNextStatus = true;
      if (line.includes(" STATUS") && window.delayNextStatus) {
        window.delayNextStatus = false;
        window.readbackDelayed = true;
        return new Promise((resolve, reject) => {
          window.finishReadback = () => Promise.resolve(write.call(this, line)).then(resolve, reject);
        });
      }
      return write.call(this, line);
    };
  });
  await page.locator("#demo").click();
  await page.locator("#supported").check();
  await page.locator("#resume").click();
  await page.locator('[data-joint="0"] .arm').click();
  await expect.poll(() => page.evaluate(() => window.readbackDelayed)).toBe(true);
  await expect(page.locator("#stop")).toBeEnabled();
  await expect(page.locator("#release")).toBeEnabled();
  await page.locator("#release").click();
  // Check transmission before releasing the ordinary readback: a queued Release is insufficient.
  await expect.poll(() => page.evaluate(() => window.sentCommands.some((line) => line.includes(" RELEASE")))).toBe(true);
  await expect(page.locator("#notice")).toContainText("Outputs released");
  await expect(page.locator('[data-joint="0"] .joint-state')).toHaveText("Detached");
  await page.evaluate(() => window.finishReadback());
  await expect(page.locator("#export")).toBeEnabled();
  await expect(page.locator("#notice")).toContainText("Outputs released");
});

test("ordinary controls wait for safety readback while STOP and Release remain available", async ({ page }) => {
  await page.evaluate(async () => {
    const { DemoTransport } = await import("/demo.js");
    const write = DemoTransport.prototype.write;
    DemoTransport.prototype.write = function (line) {
      if (line.includes(" STOP")) window.delaySafetyStatus = true;
      if (line.includes(" STATUS") && window.delaySafetyStatus) {
        window.delaySafetyStatus = false;
        window.safetyReadbackDelayed = true;
        return new Promise((resolve, reject) => {
          window.finishSafetyReadback = () => Promise.resolve(write.call(this, line)).then(resolve, reject);
        });
      }
      return write.call(this, line);
    };
  });
  await page.locator("#demo").click();
  await expect(page.locator("#export")).toBeEnabled();
  await page.locator("#stop").click();
  await expect.poll(() => page.evaluate(() => window.safetyReadbackDelayed)).toBe(true);
  await expect(page.locator("#export")).toBeDisabled({ timeout: 1000 });
  await expect(page.locator("#save")).toBeDisabled({ timeout: 1000 });
  await expect(page.locator("#disconnect")).toBeDisabled({ timeout: 1000 });
  await expect(page.locator("#stop")).toBeEnabled();
  await expect(page.locator("#release")).toBeEnabled();
  await page.evaluate(() => window.finishSafetyReadback());
  await expect(page.locator("#export")).toBeEnabled();
});

test("Release reports device rejection instead of claiming outputs detached", async ({ page }) => {
  await page.evaluate(async () => {
    const { DemoTransport } = await import("/demo.js");
    const write = DemoTransport.prototype.write;
    DemoTransport.prototype.write = function (line) {
      if (line.includes(" RELEASE"))
        return this.emit(`ERR ${line.trim().split(" ")[0]} STATE`);
      return write.call(this, line);
    };
  });
  await page.locator("#demo").click();
  await page.locator("#supported").check();
  await page.locator("#resume").click();
  await page.locator('[data-joint="0"] .arm').click();
  await expect(page.locator('[data-joint="0"] .joint-state')).toHaveText("Holding signal");
  await page.locator("#release").click();
  await expect(page.locator("#notice")).toContainText("Device: STATE");
  await expect(page.locator("#notice")).toHaveClass("error");
  await expect(page.locator('[data-joint="0"] .joint-state')).toHaveText("Holding signal");
});

test("a delayed profile completion cannot replace a newer Release result", async ({ page }) => {
  await page.evaluate(async () => {
    const { DemoTransport } = await import("/demo.js");
    const write = DemoTransport.prototype.write;
    DemoTransport.prototype.write = function (line) {
      if (line.includes(" CONFIG 11 ")) window.delayProfileStatus = true;
      if (line.includes(" STATUS") && window.delayProfileStatus) {
        window.delayProfileStatus = false;
        window.profileReadbackDelayed = true;
        return new Promise((resolve, reject) => {
          window.finishProfileReadback = () => Promise.resolve(write.call(this, line)).then(resolve, reject);
        });
      }
      return write.call(this, line);
    };
  });
  await page.locator("#demo").click();
  await page.locator("#import-file").setInputFiles({
    name: "calibration.json",
    mimeType: "application/json",
    buffer: Buffer.from(JSON.stringify({
      version: 1,
      joints: Array.from({ length: 12 }, () => ({ minimum: 1400, maximum: 1600, initial: 1500, direction: 1, calibrated: false })),
      standing: null,
      labels: ["1", "2", "3", "4"],
      front: "USB",
    })),
  });
  await page.locator("#apply-profile").click();
  await expect.poll(() => page.evaluate(() => window.profileReadbackDelayed)).toBe(true);
  await page.locator("#release").click();
  await expect(page.locator("#notice")).toContainText("Outputs released");
  await page.evaluate(() => window.finishProfileReadback());
  await expect(page.locator("#export")).toBeEnabled();
  await expect(page.locator("#notice")).toContainText("Outputs released");
});

test("unexpected device hold explains timeout and target cancellation", async ({ page }) => {
  await page.evaluate(async () => {
    const { DemoTransport } = await import("/demo.js");
    const write = DemoTransport.prototype.write;
    DemoTransport.prototype.write = function (line) {
      if (line.includes(" PING")) return this.emit(`ACK ${line.trim().split(" ")[0]}`);
      return write.call(this, line);
    };
  });
  await page.locator("#demo").click();
  await page.locator("#resume").click();
  await expect(page.locator("#motion-status")).toHaveText("All outputs detached");
  await expect(page.locator("#notice")).toContainText("Communication timeout");
  await expect(page.locator("#notice")).toContainText("cancelled");
});

test("pin labels come from device status, including Uno A0", async ({ page }) => {
  await page.evaluate(async () => {
    const { DemoTransport } = await import("/demo.js");
    const open = DemoTransport.prototype.open;
    DemoTransport.prototype.open = function (...args) {
      this.joints[11].pin = 14;
      return open.apply(this, args);
    };
  });
  await page.locator("#demo").click();
  await expect(page.locator('[data-joint="11"] .joint-id')).toContainText("A0");
  await expect(page.locator('[data-joint="0"] .joint-id')).toContainText("D2");
});
test("demo calibrates twelve joints, captures standing, stops and releases", async ({
  page,
}) => {
  await expect(
    page.getByRole("button", { name: "Capture standing pose", exact: true }),
  ).toBeDisabled();
  await page.getByRole("button", { name: "Try demo", exact: true }).click();
  await expect(page.locator("#connection")).toContainText("DEMO");
  await page
    .getByLabel("The body is supported and I checked joint retention.")
    .check();
  for (let i = 0; i < 12; i++) {
    const row = page.locator(`[data-joint="${i}"]`);
    await row.locator("summary").click();
    await row.getByLabel("Travel checked").check();
    await row.getByRole("button", { name: "Apply calibration" }).click();
    await expect(
      row.getByRole("button", { name: "Apply calibration" }),
    ).toBeEnabled();
  }
  await page
    .getByRole("button", { name: "Resume controls", exact: true })
    .click();
  for (let i = 0; i < 12; i++)
    await page
      .locator(`[data-joint="${i}"]`)
      .getByRole("button", { name: "Arm joint", exact: true })
      .click();
  await page
    .getByRole("button", { name: "Capture standing pose", exact: true })
    .click();
  await expect(page.locator("#pose-status")).toContainText("Captured");
  await page
    .getByRole("button", { name: "Move to standing", exact: true })
    .click();
  await page.getByRole("button", { name: "STOP · hold", exact: true }).focus();
  await page.keyboard.press('Enter');
  await expect(page.locator("#motion-status")).toContainText("Hold latched");
  await page
    .getByRole("button", { name: "Release outputs", exact: true })
    .click();
  await expect(
    page.locator(".joint-state").filter({ hasText: "Detached" }),
  ).toHaveCount(12);
  await page.getByRole("button", { name: "Save to Uno", exact: true }).click();
  await expect(page.locator("#notice")).toContainText("Saved");
  await page.screenshot({
    path: "test-results/console-desktop.png",
    fullPage: true,
  });
});
test("mobile controls fit, invalid import is rejected, disconnect disables motion", async ({
  page,
}) => {
  await page.setViewportSize({ width: 390, height: 844 });
  await page.getByRole("button", { name: "Try demo", exact: true }).click();
  await page.locator("#import-file").setInputFiles({
    name: "bad.json",
    mimeType: "application/json",
    buffer: Buffer.from('{"version":99}'),
  });
  await expect(page.locator("#notice")).toContainText("version 1");
  expect(
    await page.evaluate(
      () => document.documentElement.scrollWidth <= innerWidth,
    ),
  ).toBeTruthy();
  await page.screenshot({
    path: "test-results/console-mobile.png",
    fullPage: true,
  });
  await page.getByRole("button", { name: "Disconnect", exact: true }).click();
  await expect(
    page.getByRole("button", { name: "Capture standing pose", exact: true }),
  ).toBeDisabled();
  await expect(
    page
      .locator('[data-joint="0"]')
      .getByRole("button", { name: "Arm joint", exact: true }),
  ).toBeDisabled();
});
