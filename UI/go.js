var content = document.createElement("div");
content.innerHTML = `

<div id="goguide">
<img src="../Assets/goguide/back.svg"></img>
<img src="../Assets/envy.svg"></img>
<img src="../Assets/goguide/search.svg"></img>
</div>
<div id="goui">
<h1>GOUi Shell</h1>
    <div style="gap: 10px; display: flex; flex-direction: column; width: 100% !important" id="dapps">
    </div>
</div>
`;
document.getElementById("userland").appendChild(content);
// Fetch JSON and update HTML
fetch('./UI/goapps.json')
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
        <button style="border-radius: 7px !important; width: 100%" onclick="loadScript('${app.url}');">${app.name}</button>`;
        dappsContainer.appendChild(div);
    });
}

document.body.style.backgroundColor = "#202020ff";