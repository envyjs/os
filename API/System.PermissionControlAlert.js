(function() {
    const windowTitle = 'Permission Control';
    const customHTML = `
    <h1>Warning!</h1>
    <p>This application is requesting permission to access sensitive system features. Granting these permissions may pose security risks.</p>
    <p>Make sure you trust this program before granting permission.</p>
    <button onclick="grantPermissions()">Grant Permissions</button>
    <button onclick="denyPermissions()">Deny</button>
    `;
    createWindow(windowTitle, customHTML, 600, 350);
  })();