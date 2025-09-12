(function() {
  const windowTitle = 'Install Envy';
  const customHTML = `
    <div style="display: flex; height: 370px;">
      <div style="width: 24%;">
        <p style="font-size: 20px; font-weight: 900;">Introduction</p>
        <p>License</p>
        <p>Install options</p>
        <p>Install</p>
      </div>
      <div class="verticalLine" style="width: 70%; padding-left: 30px">
        <h2>Welcome to the Envy Installer</h2>
        <p>To install Envy 10 2026 Update, click Next and follow the onscreen instructions.</p>
        <p>To quit, close this window and reboot your computer.</p>
        <button class="install-button">Next</button>
      </div>
    </div>
  `;

  // Create a window dynamically
  createWindow(windowTitle, customHTML, 700, 450);
})();
