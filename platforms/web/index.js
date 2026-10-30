const canvas = document.getElementById("canvas");
canvas.addEventListener("webglcontextlost", () => location.reload());
// canvas.addEventListener("contextmenu", (event) => event.preventDefault());

Module = {
  canvas,
  print: function (text) {
    console.log(text);
  },
  printErr: function (text) {
    console.log(text);
  },
  preRun: [
    function () {
      if (window.EMBEDCONFIG) {
        FS.writeFile("/embed.txt", Uint8Array.fromBase64(window.EMBEDCONFIG));
      }
      if (window.EMBEDPAL) {
        FS.writeFile("/embed.pal", Uint8Array.fromBase64(window.EMBEDPAL));
      }
      if (window.EMBEDNES) {
        FS.writeFile("/embed.nes", Uint8Array.fromBase64(window.EMBEDNES));
      }
    },
  ],
};
