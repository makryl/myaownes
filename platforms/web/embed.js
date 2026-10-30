const fs = require("fs");
const path = require("path");

function getArg(flag) {
  const idx = process.argv.indexOf(flag);
  return idx !== -1 ? process.argv[idx + 1] : "";
}

const nesPath = getArg("--nes");
const outPath = getArg("--out") || nesPath.replace(/\.nes$/, "") + ".html";
const templatePath = getArg("--template") || path.join(__dirname, "index.html");
const configPath = getArg("--config");
const palettePath = getArg("--palette");
const title = getArg("--title") || path.basename(nesPath, ".nes");
const desc = getArg("--desc");

if (!nesPath) {
  console.log(
    "Usage: node embed.js " +
      "--nes <path/to/game.nes> " +
      "[--out <path/to/game.html>] " +
      "[--template <index.html>] " +
      "[--config <config.txt>] " +
      "[--palette <palette.pal>] " +
      "[--title <title>] " +
      "[--desc <desc>]",
  );
  process.exit(1);
}

const nesBase64 = fs.readFileSync(nesPath).toString("base64");

let config = "";
if (configPath) {
  config = fs.readFileSync(configPath, "utf8");
}
config += '\nembed "embed.nes"\n';

let paletteBase64 = "";
if (palettePath) {
  paletteBase64 = fs.readFileSync(palettePath).toString("base64");
  config += '\npalette "embed.pal"\n';
}

const configBase64 = Buffer.from(config, "utf8").toString("base64");

const html = fs
  .readFileSync(templatePath, "utf8")
  .replace(/<title>(.*)<\/title>/, `<title>${title}</title>`)
  .replace(/<meta content="(.*)"name=description>/, `<meta content="${desc}"name=description>`)
  .replace(
    "</canvas>",
    `</canvas><script>window.EMBEDCONFIG="${configBase64}";window.EMBEDPAL="${paletteBase64}";window.EMBEDNES="${nesBase64}";<\/script>`,
  );
fs.writeFileSync(outPath, html);
