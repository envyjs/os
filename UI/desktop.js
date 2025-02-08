loadScript('./Libs/bg.js')

var content = document.createElement("div");
content.innerHTML = `
<div id="desktop">
    <div class="taskbar">
        <img src="./Assets/envy.svg" onclick="semshow()" height="26px" class="smst">
        <div id="taskbarApps" class="taskbar-apps"></div>
        <div class="date">
            <div id="clock"></div>
            <div id="dateDisplay" style="font-size: 12.5px"></div>
        </div>
    </div>
    <div id="sem" style="padding: 10px;">
        <span style="display: flex; gap: 15px;">
            <img src="./Assets/kot.jpg" height="45px" style="border-radius: 99px">
            <h3 style="margin-top: 11px">User</h3>
        </span>
        <span style="max-height: 100px;">
        <div style="gap: 10px; display: flex; flex-direction: column;">
        <button style="border-radius: 7px !important" onclick="loadScript('./Apps/Cello.js'); semshow();">Cello</button>
        <button style="border-radius: 7px !important" onclick="loadScript('./Apps/Lexis.js'); semshow();">Lexis</button>
        <button style="border-radius: 7px !important" onclick="loadScript('./Apps/Linux.js'); semshow();">Linux</button>
        <button style="border-radius: 7px !important" onclick="loadScript('./Apps/Settings.js'); semshow();">Settings</button>
        <button style="border-radius: 7px !important" onclick="loadScript('./Apps/Terminal.js'); semshow();">Terminal</button>
        <button style="border-radius: 7px !important" onclick="loadScript('./Apps/WINE.js'); semshow();">WINE</button>
        </div>
        </span>
        </div>
</div>
`;
document.getElementById("userland").appendChild(content);

// Function to toggle opacity of the overlay
function semshow() {
    const overlay = document.getElementById('sem');
    if (overlay.style.opacity === '0') {
        overlay.style.opacity = '1'; // Set back to visible
    } else {
        overlay.style.opacity = '0'; // Make it fully transparent
    }
}

function timeUpdate() {
    // 24-hour time example
    let x = new Date();
    let hours = x.getHours().toString().padStart(2, "0"); // 18
    let minutes = x.getMinutes().toString().padStart(2, "0"); // 37
    let timeString = `${hours}:${minutes}`; // 18:37
    let element = document.getElementById("clock"); // assuming <... id="clock" ...>
    element.innerText = timeString;
}
timeUpdate();
setInterval(timeUpdate, 1000); // 1000ms -> 1 second delay

function formatDate() {
    const date = new Date(); // Get current date
    const daysOfWeek = ["Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"];
    const months = ["January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"];

    const dayName = daysOfWeek[date.getDay()]; // Get day of the week
    const monthName = months[date.getMonth()]; // Get month name
    const dayNumber = date.getDate(); // Get day number

    return `${dayName}, ${dayNumber} ${monthName}`;
}

function setDate() {
    const dateDisplayDiv = document.getElementById("dateDisplay"); // Get the div element
    dateDisplayDiv.textContent = formatDate(); // Set the formatted date
}

setDate();
setInterval(setDate, 1000); // 1000ms -> 1 second delay