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
      window.EMBEDNES = "%%EMBEDNES%%";
      if (window.EMBEDNES.length > 12) {
        FS.writeFile("/embed.nes", Uint8Array.fromBase64(window.EMBEDNES));
      }
    },
  ],
};
