import { test } from "@playwright/test";
import smallSnapshot from "./snapshot.json" with { type: "json" };
import bigSnapshot from "./snapshot-big.json" with { type: "json" };

// Choose snapshot via env var: BENCH_SNAPSHOT=big or small (default: small)
const useBig = process.env.BENCH_SNAPSHOT === "big";
const SNAPSHOT = (useBig ? bigSnapshot : smallSnapshot) as Record<string, string>;
const SNAPSHOT_LABEL = useBig ? "big" : "small";

interface PerfWin {
  perf: {
    bootMs: () => number;
    parses: () => number;
    bytesParsed: () => number;
    reset: () => void;
  };
}

test(`nova-gui persistence bench [${SNAPSHOT_LABEL}]`, async ({ page }) => {
  // ----- seed localStorage before the page loads
  await page.addInitScript((snap: Record<string, string>) => {
    for (const [k, v] of Object.entries(snap)) localStorage.setItem(k, v);
  }, SNAPSHOT);

  // ----- cold boot
  const t0 = Date.now();
  await page.goto("/");
  await page.waitForLoadState("networkidle");
  const wallBootMs = Date.now() - t0;

  const boot = await page.evaluate(() => {
    const w = window as unknown as PerfWin;
    return {
      appBootMs: Math.round(w.perf.bootMs()),
      parses: w.perf.parses(),
      bytesParsed: w.perf.bytesParsed(),
    };
  });
  console.log("[boot]", { wallBootMs, ...boot });

  // ----- nav phase
  await page.evaluate(() => (window as unknown as PerfWin).perf.reset());
  for (const path of [
    "/test/state",
    "/arc/space-resources/nir-spectroscopy",
    "/cameras",
    "/urc/science",
    "/",
  ]) {
    await page.goto(path);
    await page.waitForLoadState("networkidle");
  }
  const navParses = await page.evaluate(
    () => (window as unknown as PerfWin).perf.parses(),
  );
  console.log("[nav 5 routes] parses:", navParses);

  // ----- remount phase: SPA-navigate / <-> /test/state 10 times via clicking
  // a link. This keeps the JS context alive so caches persist across
  // navigations, which is what real users do.
  await page.goto("/test/state");
  await page.waitForLoadState("networkidle");
  await page.evaluate(() => (window as unknown as PerfWin).perf.reset());
  for (let i = 0; i < 10; i++) {
    await page.evaluate(() => window.history.pushState({}, "", "/"));
    await page.evaluate(() => window.dispatchEvent(new PopStateEvent("popstate")));
    await page.waitForTimeout(80);
    await page.evaluate(() => window.history.pushState({}, "", "/test/state"));
    await page.evaluate(() => window.dispatchEvent(new PopStateEvent("popstate")));
    await page.waitForTimeout(80);
  }
  const remountParses = await page.evaluate(
    () => (window as unknown as PerfWin).perf.parses(),
  );
  console.log("[SPA remount /test/state x10] parses:", remountParses);

  // ----- modal phase: open/close a modal 10x on the NIR view
  await page.goto("/arc/space-resources/nir-spectroscopy");
  await page.waitForLoadState("networkidle");
  await page.evaluate(() => (window as unknown as PerfWin).perf.reset());

  const modalButton = page
    .getByRole("button", { name: /settings|calibration|configure/i })
    .first();

  const opens = 10;
  let opened = 0;
  for (let i = 0; i < opens; i++) {
    if ((await modalButton.count()) === 0) break;
    await modalButton.click({ trial: false }).catch(() => {});
    await page.waitForTimeout(120);
    await page.keyboard.press("Escape");
    await page.waitForTimeout(120);
    opened++;
  }
  const modalParses = await page.evaluate(
    () => (window as unknown as PerfWin).perf.parses(),
  );
  console.log(`[modal x${opened}] parses:`, modalParses);

  // summary
  console.log("---- summary ----");
  console.log("wall boot ms:        ", wallBootMs);
  console.log("app boot ms:         ", boot.appBootMs);
  console.log("parses at first paint:", boot.parses);
  console.log("bytes parsed at boot:", boot.bytesParsed);
  console.log("nav parses (5 routes):", navParses);
  console.log("remount parses (x10):", remountParses);
  console.log(`modal parses (x${opened}): `, modalParses);
});
