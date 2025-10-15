let activeInput = null;
let isShift = false;

const layouts = {
  letters: [
    ['Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P'],
    ['A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L'],
    ['Shift', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', 'Backspace',],
    ['123', 'Space', 'Enter']
  ],
  numbers: [
    ['1', '2', '3', '4', '5', '6', '7', '8', '9', '0'],
    ['-', '/', ':', ';', '(', ')', '$', '&', '@', '"'],
    ['Shift', '.', ',', '?', '!', "'", 'Backspace'],
    ['ABC', 'Space', 'Enter']
  ]
};

let currentLayout = 'letters';

const keyboard = document.getElementById("keyboard");

// Show/hide keyboard based on focus
document.addEventListener('focusin', (e) => {
  if (e.target.tagName === 'INPUT' || e.target.tagName === 'TEXTAREA') {
    activeInput = e.target;
    keyboard.classList.remove('hidden');
    renderKeyboard();
  }
});

document.addEventListener('focusout', () => {
  setTimeout(() => {
    if (!document.activeElement || 
        (document.activeElement.tagName !== 'INPUT' && document.activeElement.tagName !== 'TEXTAREA')) {
      keyboard.classList.add('hidden');
      activeInput = null;
    }
  }, 100);
});

function renderKeyboard() {
  keyboard.innerHTML = ''; // Clear current keys

  layouts[currentLayout].forEach(row => {
    const rowDiv = document.createElement("div");
    rowDiv.classList.add("row");

    row.forEach(key => {
      const keyButton = document.createElement("div");
      keyButton.classList.add("key");
      if (key === "Space") keyButton.classList.add("key--space");
      else if (key === "Backspace" || key === "Enter" || key === "Shift" || key === "ABC" || key === "123") keyButton.classList.add("key--enter");

      keyButton.textContent = getKeyLabel(key);

      keyButton.addEventListener("click", () => handleKeyPress(key));

      rowDiv.appendChild(keyButton);
    });

    keyboard.appendChild(rowDiv);
  });
}

function handleKeyPress(key) {
  if (!activeInput) return;

  const start = activeInput.selectionStart;
  const end = activeInput.selectionEnd;
  const value = activeInput.value;

  switch (key) {
    case 'Space':
      insertText(' ');
      break;
    case 'Backspace':
      if (start > 0) {
        activeInput.value = value.slice(0, start - 1) + value.slice(end);
        activeInput.selectionStart = activeInput.selectionEnd = start - 1;
      }
      break;
    case 'Enter':
      if (activeInput.tagName === 'TEXTAREA') {
        insertText('\n');
      } else {
        activeInput.blur();
      }
      break;
    case '123':
      currentLayout = 'numbers';
      renderKeyboard();
      break;
    case 'ABC':
      currentLayout = 'letters';
      renderKeyboard();
      break;
    case 'Shift':
      isShift = !isShift;
      renderKeyboard();
      break;
    default:
      insertText(isShift ? key.toUpperCase() : key.toLowerCase());
      isShift = false; // one-time shift behavior
      renderKeyboard();
      break;
  }

  activeInput.dispatchEvent(new Event('input'));
  activeInput.focus();
}

function insertText(text) {
  const start = activeInput.selectionStart;
  const end = activeInput.selectionEnd;
  const value = activeInput.value;
  activeInput.value = value.slice(0, start) + text + value.slice(end);
  activeInput.selectionStart = activeInput.selectionEnd = start + text.length;
}

function getKeyLabel(key) {
  switch (key) {
    case 'Backspace': return '⌫';
    case 'Enter': return '⏎';
    case 'Shift': return '⇧';
    default:
      return isShift ? key.toUpperCase() : key.toLowerCase();
  }
}