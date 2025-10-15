var content = document.createElement("div");
content.innerHTML = `
<div id="goui">
<p>GOUI Shell v10.0.5122, running on Lumia 950<p>
    <div style="gap: 10px; display: flex; flex-direction: column; width: 100% !important" id="gapps">
    </div>
</div>
`;
document.getElementById("userland").appendChild(content);
// Fetch JSON and update HTML
fetch('./UI/goapps.json')
.then(response => response.json())
.then(data => {
    dappsData = data; // Store data globally
    displayMobileApps(data); // Display apps
})
.catch(error => console.error('Error fetching JSON:', error));

// Function to display apps
function displayMobileApps(dapps) {
    const dappsContainer = document.getElementById('gapps');
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