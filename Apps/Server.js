(function() {
  const windowTitle = 'Server Manager';
  const customHTML = `
  <center>
    <h2>Server Manager</h2>
    <p>Welcome to Envy Server.</p>
    <button id="start-server-btn">Start Server</button>
    <div id="server-status" style="margin-top: 20px; font-weight: bold;"></div>
  </center>
  `;

  // Create a window dynamically
  createWindow(windowTitle, customHTML, 600, 350);
})();