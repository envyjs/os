loadScript('./Libs/bg.js')
loadScript('./Registry/Local/Envy/StartupApps.js');

if (typeof closeLockScreen === 'function') closeLockScreen();
if (typeof closeDesktopShell === 'function') closeDesktopShell();

var desktopShellRoot = document.createElement("div");
desktopShellRoot.innerHTML = `

<audio src="./Assets/audio/boot.mp3"></audio>
<div id="desktop" style="cursor: url('./Assets/cursor/cursor.cur'),auto !important;">
    <div class="taskbar" style="cursor: url('./Assets/cursor/cursor.cur'),auto !important;">
        <div class="sm" onclick="semshow()">
        <img src="./Assets/envy.svg" style="cursor: url('./Assets/cursor/cursor.cur'),auto !important;" height="26px" class="smst">
        </div>
        <div id="taskbarApps" class="taskbar-apps"></div>
        <div class="date">
            <div style="cursor: url('./Assets/cursor/cursor.cur'),auto !important;" id="clock"></div>
            <div id="dateDisplay" style="font-size: 12.5px; cursor: url('./Assets/cursor/cursor.cur'),auto !important;"></div>
        </div>
        <div class="notibar">
        <img src="./Assets/bell.svg">
        </div>
    </div>
    <div id="sem" style="padding: 10px;">
        <span style="display: flex; gap: 15px;">
            <img src="./Assets/user/cat.jpg" height="45px" style="border-radius: 99px">
            <h3 style="margin-top: 11px">User</h3>
        <button class="gotodesktopbutton" onclick="loadScript('./API/System.DesktopToGoTransitional.js')">
        <img src="./Assets/goguide/gotodesktop.svg">
        </button>
        </span>
        <span style="max-height: 100px;">
        <div style="gap: 10px; display: flex; flex-direction: column; width: 100% !important" id="dapps">
        </div>
        </span>
    </div>
</div>
`;
document.getElementById("userland").appendChild(desktopShellRoot);

let hasClicked = false;
    document.addEventListener("click", (e) => {
      if (!hasClicked) {
        hasClicked = true;
        document.querySelector("audio").play();
      } 
    }); //startup funnies

// Function to toggle opacity of the overlay
function semshow() {
    const overlay = document.getElementById('sem');
    if (overlay.style.opacity === '0') {
        overlay.style.opacity = '1'; // Set back to visible
    } else {
        overlay.style.opacity = '0'; // Make it fully transparent
    }
}

function formatDate(date) {
    const daysOfWeek = ["Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"];
    const months = ["January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"];

    const dayName = daysOfWeek[date.getDay()]; // Get day of the week
    const monthName = months[date.getMonth()]; // Get month name
    const dayNumber = date.getDate(); // Get day number

    return `${dayName}, ${dayNumber} ${monthName}`;
}

function updateDesktopClock() {
    const date = new Date();
    const hours = date.getHours().toString().padStart(2, "0");
    const minutes = date.getMinutes().toString().padStart(2, "0");
    const timeText = `${hours}:${minutes}`;
    const clock = document.getElementById("clock");
    const dateDisplay = document.getElementById("dateDisplay");
    const dateText = formatDate(date);

    if (clock && clock.innerText !== timeText) clock.innerText = timeText;
    if (dateDisplay && dateDisplay.textContent !== dateText) dateDisplay.textContent = dateText;
}

let dappsData = []; // Store JSON data globally

// Fetch JSON and update HTML
fetch('./UI/desktopapps.json')
.then(response => response.json())
.then(data => {
    dappsData = data; // Store data globally
    displayDesktopApps(data); // Display apps
})
.catch(error => console.error('Error fetching JSON:', error));

// Function to display apps
function displayDesktopApps(dapps) {
    const dappsContainer = document.getElementById('dapps');
    dappsContainer.innerHTML = ''; // Clear previous content

    dapps.forEach(app => {
        const div = document.createElement('div');
        div.classList.add('post');
        div.innerHTML = `
        <button style="border-radius: 7px !important; width: 100%" onclick="loadScript('${app.url}'); semshow();">${app.name}</button>`;
        dappsContainer.appendChild(div);
    });
}

updateDesktopClock();
var desktopClockTimer = setInterval(updateDesktopClock, 1000);

var desktopLockShortcut = function(event) {
    if (!event.repeat && event.ctrlKey && event.metaKey && (event.key === 'q' || event.key === 'Q')) {
        event.preventDefault();
        loadScript('./UI/lock.desktop.js');
    }
};
document.addEventListener('keydown', desktopLockShortcut);

function closeDesktopShell() {
    if (desktopClockTimer) clearInterval(desktopClockTimer);
    if (desktopLockShortcut) document.removeEventListener('keydown', desktopLockShortcut);
    desktopClockTimer = 0;
    desktopLockShortcut = 0;
    if (desktopShellRoot && desktopShellRoot.parentNode) desktopShellRoot.remove();
}
