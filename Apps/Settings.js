(function() {
  const windowTitle = 'Settings';
  const customHTML = `
  <h2>Appearence</h2>
  <button onclick="loadScript('/Libs/spotlight.js')">Envy Spotlight</button>
  `;
  createWindow(windowTitle, customHTML, 600, 350);
})();