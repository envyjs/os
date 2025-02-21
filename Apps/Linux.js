(function() {
  const windowTitle = 'Linux';
  const customHTML = `
  <iframe id="emb" src="./Subsystems/Linux/index.html?cpu=x86">
  `;

  // Create a window dynamically
  createWindow(windowTitle, customHTML, 720, 580);
})();

notifier.create('Hello from the Example App', 'info', 5000);