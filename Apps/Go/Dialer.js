(function() {
  const windowTitle = 'Example App';
  const customHTML = `
  <div class="dialer">
    <input type="text" id="display" placeholder="Enter number" readonly />
    <div class="keypad">
      <button class="dkey">1</button>
      <button class="dkey">2</button>
      <button class="dkey">3</button>
      <button class="dkey">4</button>
      <button class="dkey">5</button>
      <button class="dkey">6</button>
      <button class="dkey">7</button>
      <button class="dkey">8</button>
      <button class="dkey">9</button>
      <button class="dkey">*</button>
      <button class="dkey">0</button>
      <button class="dkey">#</button>
    </div>
    <div class="actions">
      <button id="call">Call</button>
      <button id="clear">Clear</button>
    </div>
  </div>
  `;

  // Create a window dynamically
  createWindow(windowTitle, customHTML, 600, 350);
})();

notifier.create('Dialer is in VERY early alpha!!', 'info', 5000);

const display = document.getElementById("display");
const keys = document.querySelectorAll(".dkey");
const callBtn = document.getElementById("call");
const clearBtn = document.getElementById("clear");

// Function to play tone
function playTone(frequency) {
  const audioCtx = new (window.AudioContext || window.webkitAudioContext)();
  const oscillator = audioCtx.createOscillator();
  const gainNode = audioCtx.createGain();

  oscillator.type = 'sine';
  oscillator.frequency.setValueAtTime(frequency, audioCtx.currentTime);
  oscillator.connect(gainNode);
  gainNode.connect(audioCtx.destination);

  oscillator.start();
  gainNode.gain.setValueAtTime(0.1, audioCtx.currentTime); // Volume
  oscillator.stop(audioCtx.currentTime + 0.2); // Duration: 200ms
}

// Map keypad values to frequencies (basic example)
const toneMap = {
  '1': 697,
  '2': 770,
  '3': 852,
  '4': 941,
  '5': 1209,
  '6': 1336,
  '7': 1477,
  '8': 1633,
  '9': 880,
  '0': 1000,
  '*': 1100,
  '#': 1200
};

// Add number to display on key press
keys.forEach(key => {
  key.addEventListener("click", () => {
    const value = key.textContent;
    display.value += value;

    // Play tone if frequency is mapped
    if (toneMap[value]) {
      playTone(toneMap[value]);
    }
  });
});

// Simulate call
callBtn.addEventListener("click", () => {
  const number = display.value;
  if (number) {
    alert(`Calling ${number}...`);
    display.value = "";
  } else {
    alert("Please enter a number.");
  }
});

// Clear display
clearBtn.addEventListener("click", () => {
  display.value = "";
});
