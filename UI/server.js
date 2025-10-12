(function() {
  const windowTitle = 'Server Manager';
  const customHTML = `
  <center>
  <h2>Apps</h2>
  <button onclick="loadScript('./Apps/Terminal.js');">Terminal</button>
  <button onclick="loadScript('./Apps/Filer.js');">Filer</button>
  <h2>Accounts</h2>
  <button>Manage users</button>
  <button>Manage groups</button>
  <h2>Services</h2>
  <button>File Sharing (Envy)</button>
  <button>Address Book</button>
  <h2>Information</h2>
  <button onclick="loadScript('./Apps/About.js');">System Information</button>
  <button onclick="loadScript('./Apps/Server/Log.js');">Log Files</button>
  <button onclick="loadScript('./Apps/Server/NI.js');">Network Information</button>
  </center>
  `;

  // Create a window dynamically
  createWindow(windowTitle, customHTML, 430, 500);
})();
document.body.style.backgroundColor = "#0f0f0fff";
document.querySelectorAll('.window-minimize-btn').forEach(el => el.style.display = 'none');
document.querySelectorAll('.window-maximize-btn').forEach(el => el.style.display = 'none');
document.querySelectorAll('.window-close-btn').forEach(el => el.style.display = 'none');