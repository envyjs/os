(function() {
  const windowTitle = 'Settings';
  const customHTML = `
  <h2>Appearence</h2>
  <button onclick="loadScript('/Libs/spotlight.js')">Envy Spotlight</button>
  `;
  createWindow(windowTitle, customHTML, [
    { label: 'Close', action: closeWindow },
    { label: 'Minimize', action: minimizeWindow },
    { label: 'Maximize', action: maximizeWindow }
  ]);
})();