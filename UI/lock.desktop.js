if (typeof closeDesktopShell === 'function') closeDesktopShell();
if (typeof closeLockScreen === 'function') closeLockScreen();

var lockScreenRoot = document.createElement("div");
lockScreenRoot.innerHTML = `
    <div id="lock" onclick="loadScript('./Registry/Local/Envy/DefaultShell/Desktop.js');">
    <div class="ld">
        <div id="lockdateDisplay"></div>
    </div>  
        <div class="lt">
            <div id="lockclock">
        </div>
    </div>
`;
document.getElementById("userland").appendChild(lockScreenRoot);

function formatDate(date) {
    const daysOfWeek = ["Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"];
    const months = ["January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"];

    const dayName = daysOfWeek[date.getDay()]; // Get day of the week
    const monthName = months[date.getMonth()]; // Get month name
    const dayNumber = date.getDate(); // Get day number

    return `${dayName}, ${dayNumber} ${monthName}`;
}

function updateLockClock() {
    const date = new Date();
    const hours = date.getHours().toString().padStart(2, "0");
    const minutes = date.getMinutes().toString().padStart(2, "0");
    const timeText = `${hours}\n    ${minutes}`;
    const clock = document.getElementById("lockclock");
    const dateDisplay = document.getElementById("lockdateDisplay");
    const dateText = formatDate(date);

    if (clock && clock.innerText !== timeText) clock.innerText = timeText;
    if (dateDisplay && dateDisplay.textContent !== dateText) dateDisplay.textContent = dateText;
}

updateLockClock();
var lockClockTimer = setInterval(updateLockClock, 1000);
var lockDateTimer = 0;

function closeLockScreen() {
    if (lockClockTimer) clearInterval(lockClockTimer);
    if (lockDateTimer) clearInterval(lockDateTimer);
    lockClockTimer = 0;
    lockDateTimer = 0;
    if (lockScreenRoot && lockScreenRoot.parentNode) lockScreenRoot.remove();
}
