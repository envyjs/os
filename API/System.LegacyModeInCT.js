(function() {
    const windowTitle = 'Legacy mode (2024 Update)';
    const customHTML = `
    <iframe id="emb" src="../Subsystems/ct/index.html">
    `;
  
    // Create a window dynamically
    createWindow(windowTitle, customHTML, 800, 600);
  })();