(function() {
    const windowTitle = 'LiveCD Notice';
    const customHTML = `
    <h2>Caution!</h2>
    <p>Welcome to the LiveCD for Envy. Please note that no data will be kept, as this is a LIVE enviornment.</p>
    `;
    createWindow(windowTitle, customHTML, 600, 200);
  })();