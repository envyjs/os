(function() {
  const windowTitle = 'Linux';
  const customHTML = `
  <iframe id="emb" src="./Subsystems/Linux/index.html?cpu=x86">
  `;

  // Create a window dynamically
  createWindow(windowTitle, customHTML, [
    { label: 'Close', action: closeWindow },
    { label: 'Minimize', action: minimizeWindow },
    { label: 'Maximize', action: maximizeWindow }
  ]);
})();

notifier.create('Hello from the Example App', 'info', 5000);