(function() {
    const windowTitle = 'Legacy mode (8.0.1)';
    const customHTML = `
    <iframe id="emb" src="../Subsystems/legacy/index.html">
    `;
  
    // Create a window dynamically
    createWindow(windowTitle, customHTML, 800, 600);
  })();