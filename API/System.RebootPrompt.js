(function() {
    const windowTitle = 'Reboot Envy?';
    const customHTML = `
    <h2>Alert</h2>
    <p>In order for the effects to take place, Envy needs to reboot. Reboot?</p>
    <button onclick="location.reload();">Yes</button>
    `;
    createWindow(windowTitle, customHTML, 560, 230);
  })();