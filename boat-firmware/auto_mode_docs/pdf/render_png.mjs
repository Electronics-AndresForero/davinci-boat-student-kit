// Saca una PNG limpia (solo el SVG, sin botones del visor) de cada diagrama Archify.
// Uso: node render_png.mjs   (requiere puppeteer-core de mermaid-cli y ARCHIFY_CHROME)
import { createRequire } from 'module';
import path from 'path'; import os from 'os'; import fs from 'fs';
const require = createRequire(path.join(os.homedir(), '.local/share/mermaid-cli/'));
const puppeteer = require('puppeteer-core');
const here = path.dirname(new URL(import.meta.url).pathname);
const dir = path.join(here, '..', 'diagramas');
const chrome = process.env.ARCHIFY_CHROME;
if (!chrome) { console.error('Define ARCHIFY_CHROME'); process.exit(1); }
const browser = await puppeteer.launch({ executablePath: chrome, args: ['--no-sandbox'] });
for (const n of ['estados-auto', 'ciclo-loop', 'log-datos']) {
  const page = await browser.newPage();
  await page.setViewport({ width: 1600, height: 1000, deviceScaleFactor: 3 });
  await page.emulateMediaFeatures([{ name: 'prefers-color-scheme', value: 'light' }]);
  await page.goto('file://' + path.join(dir, `${n}.archify.html`));
  await page.addStyleTag({ content: '.diagram-nav,.no-print{display:none !important}' });
  await page.evaluate(() => { for (const e of document.querySelectorAll('body *')) { const p = getComputedStyle(e).position; if ((p === 'fixed' || p === 'sticky') && !e.querySelector('svg[role="img"]')) e.style.display = 'none'; } });
  const svg = await page.$('svg[role="img"]');
  await svg.screenshot({ path: path.join(here, `${n}.png`), omitBackground: false });
  console.log(n, 'ok');
  await page.close();
}
await browser.close();
