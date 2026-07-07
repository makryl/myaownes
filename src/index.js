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
};
