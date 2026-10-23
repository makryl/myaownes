const fs = require("fs");
const path = require("path");

function getArg(flag) {
  const idx = process.argv.indexOf(flag);
  return idx !== -1 ? process.argv[idx + 1] : "";
}

const nesPath = getArg("--nes");
const outPath = getArg("--out") || nesPath.replace(/\.nes$/, "") + ".html";
const templatePath = getArg("--template") || path.join(__dirname, "index.html");
const title = getArg("--title") || path.basename(nesPath, ".nes");
const desc = getArg("--desc");

if (!nesPath) {
  console.log(
    "Usage: node embed.js " +
      "--nes <path/to/game.nes> " +
      "[--out <path/to/game.html>] " +
      "[--template <index.html>] " +
      "[--title <title>] " +
      "[--desc <desc>]",
  );
  process.exit(1);
}

const nes = fs.readFileSync(nesPath).toString("base64");

const html = fs
  .readFileSync(templatePath, "utf8")
  .replace(/<title>(.*)<\/title>/, `<title>${title}</title>`)
  .replace(/<meta content="(.*)"name=description>/, `<meta content="${desc}"name=description>`)
  .replace("%%EMBEDNES%%", nes);
fs.writeFileSync(outPath, html);
