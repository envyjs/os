window.addEventListener("gamepadconnected", () => {
    console.log("[GP] Gamepad connected");
    loadScript('./UI/guide.js');
});
console.log("[GP] Correr gamepad library loaded")