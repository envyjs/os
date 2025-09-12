loadScript('./Libs/bg.js')

var content = document.createElement("div");
content.innerHTML = `
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
            <img src="./Assets/kot.jpg" height="45px" style="border-radius: 99px">
            <h3 style="margin-top: 11px">User</h3>
        </span>
        <span style="max-height: 100px;">
        <div style="gap: 10px; display: flex; flex-direction: column; width: 100% !important" id="dapps">
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

setDate();
setInterval(setDate, 1000); // 1000ms -> 1 second delay