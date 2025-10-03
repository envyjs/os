(function() {
  const windowTitle = 'System Log';
  const customHTML = `
    <iframe src="./Registry/Local/Log.txt" style="width:100%; height:100%; border:none; background-color: white;"></iframe>
  `;

  // Create a window dynamically
  createWindow(windowTitle, customHTML, 600, 350);
})();