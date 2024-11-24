var content = document.createElement("div");
content.innerHTML = `
    <div id="lock" onclick="loadScript('./UI/desktop.js');">
     <p style="font-size: 22px" class="ut">Press any key to unlock</p>
    <div class="ld">
        <div id="lockdateDisplay"></div>
    </div>  
        <div class="lt">
            <div id="lockclock">
        </div>
    </div>
`;
document.getElementById("userland").appendChild(content);

function timeUpdate() {
    // 24-hour time example
    let x = new Date();
    let hours = x.getHours().toString().padStart(2, "0"); // 18
    let minutes = x.getMinutes().toString().padStart(2, "0"); // 37
    let timeString = `${hours}
    ${minutes}`; // 18:37
    let element = document.getElementById("lockclock"); // assuming <... id="clock" ...>
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
    const dateDisplayDiv = document.getElementById("lockdateDisplay"); // Get the div element
    dateDisplayDiv.textContent = formatDate(); // Set the formatted date
}

setDate();
setInterval(setDate, 1000); // 1000ms -> 1 second delay