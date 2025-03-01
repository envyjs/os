(function() {
  const windowTitle = 'Envy Store';
  const customHTML = `
  <input type="text" id="appsearch" placeholder="Search apps" onkeyup="filterApps()">
  <div style="display: flex;">
    <div style="width: 30%">
    <button onclick="document.getElementById('appsearch').value = 'AI'; filterApps()">
    AI
    </button>
    <br><br>
    <button onclick="document.getElementById('appsearch').value = 'Subsystems'; filterApps()">
    Subsystems
    </button>
    </div>
    <div style="width: 70%">
      <div id="apps"></div>
    </div>
  </div>
  `;

  createWindow(windowTitle, customHTML, 770, 450);
})();

let appsData = []; // Store JSON data globally

// Fetch JSON and update HTML
fetch('https://store.envy.ink/guide/apps.json')
.then(response => response.json())
.then(data => {
    appsData = data; // Store data globally
    displayApps(data); // Display apps
})
.catch(error => console.error('Error fetching JSON:', error));

// Function to display apps
function displayApps(apps) {
    const appsContainer = document.getElementById('apps');
    appsContainer.innerHTML = ''; // Clear previous content

    apps.forEach(app => {
        const div = document.createElement('div');
        div.classList.add('post');
        div.innerHTML = `
        <button id="storebutton" onclick="loadScript('${app.app}');">
          <div style="display: flex; align-items: center;">
            <span>
              <img height="55px" src="${app.icon}">
            </span>
            <span style="padding-left: 10px;">
              <h2>${app.title}</h2>
              <p>${app.desc}</p>
            </span>
            <p style="opacity: 0 !important;">${app.type}</p>
          </div>
        </button>
        <br><br>`;
        appsContainer.appendChild(div);
    });
}

// Function to filter apps based on search input
function filterApps() {
    const searchText = document.getElementById('appsearch').value.toLowerCase();
    const filteredApps = appsData.filter(app => 
        app.title.toLowerCase().includes(searchText) || 
        app.desc.toLowerCase().includes(searchText) || // Fixed from 'content' to 'desc'
        app.type.toLowerCase().includes(searchText)
    );
    displayApps(filteredApps); // Update UI
}

notifier.create('Store v3 is in ALPHA state. EXPECT BUGS.', 'info', 5000);
