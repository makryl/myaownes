const fs = require("fs");
const path = require("path");

function getArg(flag) {
  const idx = process.argv.indexOf(flag);
  return idx !== -1 ? process.argv[idx + 1] : "";
}

const nesPath = getArg("--nes");
const outPath = getArg("--out");
const title = getArg("--title");
const desc = getArg("--desc");

if (!nesPath || !outPath) {
  console.log(`Usage: node embed.js --nes <path/to/game.nes> --out <path/to/dir> [--title <title>] [--desc <desc>]`);
  process.exit(1);
}

fs.mkdirSync(outPath, { recursive: true });

const nes = fs.readFileSync(nesPath).toString("base64");
const js = fs.readFileSync(path.join(__dirname, "index.js"), "utf8").replace("%%EMBEDNES%%", nes);
fs.writeFileSync(path.join(outPath, "index.js"), js);

const html = fs
  .readFileSync(path.join(__dirname, "index.html"), "utf8")
  .replace(/<title>(.*)<\/title>/, `<title>${title}</title>`)
  .replace(/<meta content="(.*)"name=description>/, `<meta content="${desc}"name=description>`);
fs.writeFileSync(path.join(outPath, "index.html"), html);

fs.copyFileSync(path.join(__dirname, "index.wasm"), path.join(outPath, "index.wasm"));
