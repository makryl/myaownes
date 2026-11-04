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
const atlasPath = getArg("--atlas");
const title = (getArg("--title") || path.basename(nesPath, ".nes")).replace(/[<>:"/\\|?*]/g, "").substring(0, 128);
const desc = getArg("--desc");

if (!nesPath) {
  console.log(
    "Usage: node embed.js " +
      "--nes <path/to/game.nes> " +
      "[--out <path/to/game.html>] " +
      "[--template <path/to/index.html>] " +
      "[--config <path/to/config.txt>] " +
      "[--palette <path/to/palette.pal>] " +
      "[--atlas <path/to/atlas.png>] " +
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

let paletteBase64 = "";
if (palettePath) {
  paletteBase64 = fs.readFileSync(palettePath).toString("base64");
  config += '\npalette "embed.pal"\n';
}

let atlasBase64 = "";
if (atlasPath) {
  atlasBase64 = fs.readFileSync(atlasPath).toString("base64");
  config += '\natlas "embed.png"\n';
}

const configBase64 = Buffer.from(config, "utf8").toString("base64");

const html = fs
  .readFileSync(templatePath, "utf8")
  .replace(/<title>(.*)<\/title>/, `<title>${title}</title>`)
  .replace(/<meta content="(.*)"name=description>/, `<meta content="${desc}"name=description>`)
  .replace(
    "</canvas>",
    `</canvas><script>Module={arguments:["--embed","/${title}.nes","--config","/embed.txt","--path","/storage/${title}/"],preRun:[function(){FS.writeFile("/embed.txt",Uint8Array.fromBase64("${configBase64}"));var p="${paletteBase64}";p&&FS.writeFile("/embed.pal",Uint8Array.fromBase64(p));var a="${atlasBase64}";a&&FS.writeFile("/embed.png",Uint8Array.fromBase64(a));FS.writeFile("/${title}.nes",Uint8Array.fromBase64("${nesBase64}"))}]}<\/script>`,
  );
fs.writeFileSync(outPath, html);
