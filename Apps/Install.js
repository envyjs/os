(function() {
  const windowTitle = 'Install Envy';
  const customHTML = `
    <div style="display: flex;">
      <div style="width: 30%">
        <p style="font-size: 20px; font-weight: 900;">Introduction</p>
        <p>License</p>
        <p>Install options</p>
        <p>Install</p>
      </div>
      <div style="width: 70%">
        <h2>Welcome to the Envy Installer</h2>
        <button>Next</button>
      </div>
    </div>
  `;

  // Create a window dynamically
  createWindow(windowTitle, customHTML, 700, 450);
})();
