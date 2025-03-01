var content = document.createElement("div");
content.innerHTML = `
<div class="mini-guide" id="miniGuide">
    <button>Start Dashboard</button>
    <br><br>
    <button>Quickplay</button>
    <br><br>
    <button>Friends (0 online)</button>
    <br><br>
    <button>Messages (0 unread)</button>
        <img class="guidelogo" src="./Assets/envy.svg" height="40px">
        <img class="guidegame" src="./Assets/dashboard/joystick.svg">
        <img class="guideset" src="./Assets/dashboard/settings.svg">
</div>
`;
document.getElementById("userland").appendChild(content);

let isVisible = false;
let buttonPressed = false;

function checkGamepad() {
    let gamepads = navigator.getGamepads();
    if (gamepads[0]) {
        const buttons = gamepads[0].buttons;
        if (buttons[16].pressed && !buttonPressed) {
            guideActivate();
            buttonPressed = true;
        } else if (!buttons[16].pressed) {
            buttonPressed = false;
        }
    }
}

// Run checkGamepad every 100ms
setInterval(checkGamepad, 100);

function guideActivate() {
    let div = document.getElementById("miniGuide");
    isVisible = !isVisible;
    div.style.display = isVisible ? "block" : "none";
}
