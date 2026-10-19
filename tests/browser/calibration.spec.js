import { test, expect } from "@playwright/test";
test.beforeEach(async ({ page }) => {
  page.on("dialog", (dialog) => dialog.accept());
  await page.goto("/");
});
test("USB-only servo supply warning is visible before arming", async ({
  page,
}) => {
  await expect(
    page
      .locator("aside")
      .getByText("USB-only servo power is unsuitable.", { exact: true }),
  ).toBeVisible();
});
test("acknowledged initial calibration becomes the explicit arming pulse", async ({
  page,
}) => {
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

test("Release bypasses a busy action and its acknowledgement remains visible", async ({
  page,
}) => {
  await page.evaluate(async () => {
    const { DemoTransport } = await import("/demo.js?v=3.2");
    const write = DemoTransport.prototype.write;
    DemoTransport.prototype.write = function (line) {
      (window.sentCommands ??= []).push(line);
      if (line.includes(" ARM ")) window.delayNextStatus = true;
      if (line.includes(" STATUS") && window.delayNextStatus) {
        window.delayNextStatus = false;
        window.readbackDelayed = true;
        return new Promise((resolve, reject) => {
          window.finishReadback = () =>
            Promise.resolve(write.call(this, line)).then(resolve, reject);
        });
      }
      return write.call(this, line);
    };
  });
  await page.locator("#demo").click();
  await page.locator("#supported").check();
  await page.locator("#resume").click();
  await page.locator('[data-joint="0"] .arm').click();
  await expect
    .poll(() => page.evaluate(() => window.readbackDelayed))
    .toBe(true);
  await expect(page.locator("#stop")).toBeEnabled();
  await expect(page.locator("#release")).toBeEnabled();
  await page.locator("#release").click();
  // Check transmission before releasing the ordinary readback: a queued Release is insufficient.
  await expect
    .poll(() =>
      page.evaluate(() =>
        window.sentCommands.some((line) => line.includes(" RELEASE")),
      ),
    )
    .toBe(true);
  await expect(page.locator("#notice")).toContainText("Outputs released");
  await expect(page.locator('[data-joint="0"] .joint-state')).toHaveText(
    "Detached",
  );
  await page.evaluate(() => window.finishReadback());
  await expect(page.locator("#export")).toBeEnabled();
  await expect(page.locator("#notice")).toContainText("Outputs released");
});

test("ordinary controls wait for safety readback while STOP and Release remain available", async ({
  page,
}) => {
  await page.evaluate(async () => {
    const { DemoTransport } = await import("/demo.js?v=3.2");
    const write = DemoTransport.prototype.write;
    DemoTransport.prototype.write = function (line) {
      if (line.includes(" STOP")) window.delaySafetyStatus = true;
      if (line.includes(" STATUS") && window.delaySafetyStatus) {
        window.delaySafetyStatus = false;
        window.safetyReadbackDelayed = true;
        return new Promise((resolve, reject) => {
          window.finishSafetyReadback = () =>
            Promise.resolve(write.call(this, line)).then(resolve, reject);
        });
      }
      return write.call(this, line);
    };
  });
  await page.locator("#demo").click();
  await expect(page.locator("#export")).toBeEnabled();
  await page.locator("#stop").click();
  await expect
    .poll(() => page.evaluate(() => window.safetyReadbackDelayed))
    .toBe(true);
  await expect(page.locator("#export")).toBeDisabled({ timeout: 1000 });
  await expect(page.locator("#save")).toBeDisabled({ timeout: 1000 });
  await expect(page.locator("#disconnect")).toBeDisabled({ timeout: 1000 });
  await expect(page.locator("#stop")).toBeEnabled();
  await expect(page.locator("#release")).toBeEnabled();
  await page.evaluate(() => window.finishSafetyReadback());
  await expect(page.locator("#export")).toBeEnabled();
});

test("Release reports device rejection instead of claiming outputs detached", async ({
  page,
}) => {
  await page.evaluate(async () => {
    const { DemoTransport } = await import("/demo.js?v=3.2");
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
  await expect(page.locator('[data-joint="0"] .joint-state')).toHaveText(
    "Holding signal",
  );
  await page.locator("#release").click();
  await expect(page.locator("#notice")).toContainText("Device: STATE");
  await expect(page.locator("#notice")).toHaveClass("error");
  await expect(page.locator('[data-joint="0"] .joint-state')).toHaveText(
    "Holding signal",
  );
});

test("a delayed profile completion cannot replace a newer Release result", async ({
  page,
}) => {
  await page.evaluate(async () => {
    const { DemoTransport } = await import("/demo.js?v=3.2");
    const write = DemoTransport.prototype.write;
    DemoTransport.prototype.write = function (line) {
      if (line.includes(" CONFIG 11 ")) window.delayProfileStatus = true;
      if (line.includes(" STATUS") && window.delayProfileStatus) {
        window.delayProfileStatus = false;
        window.profileReadbackDelayed = true;
        return new Promise((resolve, reject) => {
          window.finishProfileReadback = () =>
            Promise.resolve(write.call(this, line)).then(resolve, reject);
        });
      }
      return write.call(this, line);
    };
  });
  await page.locator("#demo").click();
  await page.locator("#import-file").setInputFiles({
    name: "calibration.json",
    mimeType: "application/json",
    buffer: Buffer.from(
      JSON.stringify({
        version: 1,
        joints: Array.from({ length: 12 }, () => ({
          minimum: 1400,
          maximum: 1600,
          initial: 1500,
          direction: 1,
          calibrated: false,
        })),
        standing: null,
        labels: ["1", "2", "3", "4"],
        front: "USB",
      }),
    ),
  });
  await page.locator("#apply-profile").click();
  await expect
    .poll(() => page.evaluate(() => window.profileReadbackDelayed))
    .toBe(true);
  await page.locator("#release").click();
  await expect(page.locator("#notice")).toContainText("Outputs released");
  await page.evaluate(() => window.finishProfileReadback());
  await expect(page.locator("#export")).toBeEnabled();
  await expect(page.locator("#notice")).toContainText("Outputs released");
});

test("unexpected device hold explains timeout and target cancellation", async ({
  page,
}) => {
  await page.evaluate(async () => {
    const { DemoTransport } = await import("/demo.js?v=3.2");
    const write = DemoTransport.prototype.write;
    DemoTransport.prototype.write = function (line) {
      if (line.includes(" PING"))
        return this.emit(`ACK ${line.trim().split(" ")[0]}`);
      return write.call(this, line);
    };
  });
  await page.locator("#demo").click();
  await page.locator("#resume").click();
  await expect(page.locator("#motion-status")).toHaveText(
    "All outputs detached",
  );
  await expect(page.locator("#notice")).toContainText("Communication timeout");
  await expect(page.locator("#notice")).toContainText("cancelled");
});

test("pin labels come from device status, including Uno A0", async ({
  page,
}) => {
  await page.evaluate(async () => {
    const { DemoTransport } = await import("/demo.js?v=3.2");
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
  await page.keyboard.press("Enter");
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

test("unloaded servo controls require a separate confirmation and explicit Resume", async ({
  page,
}) => {
  await expect(
    page.getByRole("button", { name: "Center 90°", exact: false }),
  ).toBeVisible();
  await expect(page.locator("#bench-center")).toBeDisabled();
  await page.locator("#demo").click();
  await page.locator("#supported").check();
  await page.locator("#resume").click();
  await expect(page.locator("#bench-center")).toBeDisabled();
  await page.locator("#unloaded").check();
  await expect(page.locator("#bench-center")).toBeEnabled();
  await expect(page.locator("#bench-sweep")).toBeDisabled();
  await expect(page.locator("#bench-joint option")).toHaveCount(12);
  await expect(page.locator("#bench-joint option").last()).toContainText("D13");
  await page.locator("#stop").click();
  await expect(page.locator("#bench-center")).toBeDisabled();
  await page.locator("#resume").click();
  await page.locator('[data-joint="0"] .arm').click();
  await expect(page.locator("#bench-center")).toBeDisabled();
  await page.locator("#release").click();
  await expect(page.locator("#bench-center")).toBeEnabled();
});

test("nominal center attaches only the chosen servo and preserves the original leg limits", async ({
  page,
}) => {
  await page.locator("#demo").click();
  const row = page.locator('[data-joint="4"]');
  await row.locator("summary").click();
  await row.getByLabel("Maximum (µs)").fill("1800");
  await row.getByLabel("Initial pulse (µs)").fill("1750");
  await row.getByLabel("Minimum (µs)").fill("1700");
  await row.getByLabel("Travel checked").check();
  await row.getByRole("button", { name: "Apply calibration" }).click();
  await page.locator("#bench-joint").selectOption("4");
  await page.locator("#unloaded").check();
  await expect(page.locator("#bench-center")).toBeDisabled();
  await page.locator("#resume").click();
  await page.locator("#bench-center").click();
  await expect(page.locator("#bench-commanded")).toHaveText("1500");
  await expect(page.locator("#bench-target")).toHaveText("1500");
  await expect(page.locator("#bench-status")).toContainText("Centered");
  await expect(page.locator("#bench-joint")).toBeDisabled();
  await expect(page.locator("#bench-sweep")).toBeEnabled();
  await expect(page.locator("#output-count")).toHaveText("1 / 12 attached");
  await expect(
    page.locator(".joint-state").filter({ hasText: "Detached" }),
  ).toHaveCount(11);
  await expect(row.getByLabel("Minimum (µs)")).toHaveValue("1700");
  await expect(row.getByLabel("Maximum (µs)")).toHaveValue("1800");
  await expect(row.getByLabel("Initial pulse (µs)")).toHaveValue("1750");
  await expect(row.getByLabel("Travel checked")).not.toBeChecked();
  await expect(
    page.locator(".arm:enabled, .move:enabled, .minus:enabled, .plus:enabled"),
  ).toHaveCount(0);
  await expect(page.locator("#export")).toBeDisabled();
  await expect(page.locator("#capture")).toBeDisabled();
  await expect(page.locator("#stand")).toBeDisabled();
  await page.locator("#release").click();
  await expect(page.locator("#bench-joint")).toBeEnabled();
  await page.locator("#bench-joint").selectOption("5");
  await expect(page.locator("#bench-sweep")).toBeDisabled();
});

test("unloaded sweep validates limits, reports progress, and STOP requires explicit recentering", async ({
  page,
}) => {
  await page.clock.install();
  await page.locator("#demo").click();
  await page.locator("#unloaded").check();
  await page.locator("#resume").click();
  await page.locator("#bench-center").click();
  await page.locator("#bench-minimum").fill("1500");
  await expect(page.locator("#bench-sweep")).toBeDisabled();
  await page.locator("#bench-minimum").fill("1400");
  await page.locator("#bench-maximum").fill("1600");
  await page.locator("#bench-speed").fill("101");
  await expect(page.locator("#bench-sweep")).toBeDisabled();
  await page.locator("#bench-speed").fill("50");
  await page.locator("#bench-sweep").click();
  await expect(page.locator("#bench-status")).toContainText("minimum");
  await expect(page.locator("#bench-sweep")).toBeDisabled();
  await expect(page.locator("#bench-center")).toBeEnabled();
  await page.clock.runFor(800);
  await expect(page.locator("#bench-commanded")).not.toHaveText("1500");
  await page.locator("#stop").click();
  const held = await page.locator("#bench-commanded").textContent();
  await expect(page.locator("#bench-status")).toContainText("Hold");
  await expect(page.locator("#bench-center")).toBeDisabled();
  await page.locator("#resume").click();
  await page.clock.runFor(800);
  await expect(page.locator("#bench-commanded")).toHaveText(held);
  await expect(page.locator("#bench-sweep")).toBeDisabled();
  await page.locator("#bench-center").click();
  await expect(page.locator("#bench-status")).toContainText("Recentering");
  await page.clock.runFor(800);
  await expect(page.locator("#bench-commanded")).toHaveText("1500");
  await expect(page.locator("#bench-sweep")).toBeEnabled();
  await page.locator("#bench-sweep").click();
  await page.locator("#release").click();
  await expect(page.locator("#bench-status")).toContainText("Released");
  await expect(page.locator("#output-count")).toHaveText("0 / 12 attached");
});

test("connecting to an existing unloaded hold selects the active device servo", async ({
  page,
}) => {
  await page.evaluate(async () => {
    const { DemoTransport } = await import("/demo.js?v=3.2");
    const open = DemoTransport.prototype.open;
    DemoTransport.prototype.open = function (...args) {
      this.benchJoint = 7;
      this.joints[7].attached = 1;
      this.joints[7].pulse = this.joints[7].target = 1475;
      return open.apply(this, args);
    };
  });
  await page.locator("#demo").click();
  await expect(page.locator("#bench-joint")).toHaveValue("7");
  await expect(page.locator("#bench-joint")).toBeDisabled();
  await expect(page.locator("#bench-commanded")).toHaveText("1475");
  await page.locator("#unloaded").check();
  await page.locator("#resume").click();
  await expect(page.locator("#bench-center")).toBeEnabled();
});

for (const kind of ["version", "read", "healthy"]) {
  test(`serial startup ${kind} preserves the useful result after the reset wait`, async ({
    page,
  }) => {
    await page.evaluate(async (kind) => {
      const { DemoTransport } = await import("/demo.js?v=3.2");
      const firmware = new DemoTransport();
      const port = {
        async open() {
          let inbound;
          this.readable = new ReadableStream({
            start(controller) {
              inbound = controller;
            },
          });
          this.writable = new WritableStream({
            async write(bytes) {
              const line = new TextDecoder().decode(bytes);
              (window.serialWrites ??= []).push(line);
              await firmware.write(line);
            },
          });
          if (kind === "read")
            inbound.error(new Error("Serial read failed during board reset"));
          else {
            await firmware.open((line) =>
              inbound.enqueue(new TextEncoder().encode(line + "\n")),
            );
            inbound.enqueue(
              new TextEncoder().encode(
                kind === "version" ? "BOOT SPYDER 2\n" : "BOOT SPYDER 3\n",
              ),
            );
          }
        },
        async close() {
          await firmware.close();
        },
      };
      Object.defineProperty(navigator, "serial", {
        configurable: true,
        value: { requestPort: async () => port },
      });
    }, kind);
    await page.locator("#connect").click();
    if (kind === "healthy") {
      await expect(page.locator("#connection")).toHaveText(
        "UNO · USB connected",
      );
      await expect(page.locator("#output-count")).toHaveText("0 / 12 attached");
      await expect(page.locator("#motion-status")).toContainText(
        "Hold latched",
      );
      await expect(page.locator("#bench-center")).toBeDisabled();
      await page.locator("#disconnect").click();
    } else {
      await expect(page.locator("#disconnect")).toBeEnabled();
      await expect(page.locator("#notice")).toContainText(
        kind === "version"
          ? "Unsupported firmware"
          : "Serial read failed during board reset",
      );
      await expect(page.locator("#notice")).not.toContainText(
        "Connection cancelled",
      );
      expect(await page.evaluate(() => window.serialWrites ?? [])).toEqual([]);
      await expect(page.locator("#bench-center")).toBeDisabled();
    }
  });
}

test("current console loads coherently when obsolete unversioned modules exist", async ({
  page,
}) => {
  const obsolete = [];
  await page.route(
    (url) =>
      /\/(app|session|protocol|demo)\.js$/.test(url.pathname) && !url.search,
    async (route) => {
      obsolete.push(route.request().url());
      await route.fulfill({
        contentType: "text/javascript",
        body: 'throw new Error("Obsolete revision 2 module was reused");',
      });
    },
  );
  await page.goto("/?console=3.2");
  await expect(page.locator("#console-version")).toHaveText(
    "Console 3.2 · Firmware 3",
  );
  await page.locator("#demo").click();
  await expect(page.locator("#connection")).toHaveText(
    "DEMO · simulated robot",
  );
  await page.locator("#unloaded").check();
  await page.locator("#resume").click();
  await page.locator("#bench-center").click();
  await expect(page.locator("#bench-commanded")).toHaveText("1500");
  expect(obsolete).toEqual([]);
});
