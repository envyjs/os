(function() {
  const windowTitle = 'Server Manager';
  const customHTML = `
  <center>
  <h2>Apps</h2>
  <button onclick="loadScript('./Apps/Terminal.js');">Terminal</button>
  <button onclick="loadScript('./Apps/Filer.js');">Filer</button>
  <button onclick="loadScript('./Apps/Settings.js');">Settings</button>
  <h2>Accounts</h2>
  <button>Manage users</button>
  <button>Manage groups</button>
  <h2>Services</h2>
  <button>File Sharing (Envy)</button>
  <button>Address Book</button>
  <h2>Information</h2>
  <button onclick="loadScript('./Apps/AboutCanary.js');">System Information</button>
  <button>Log Files</button>
  <button>Network Information</button>
  </center>
  `;

  // Create a window dynamically
  createWindow(windowTitle, customHTML, 650, 500);
})();
document.body.style.backgroundImage = "url('../Assets/wallpaper/beta/wp.jpg')"; 