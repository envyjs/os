(function() {
    const windowTitle = 'Create new folder';
    const customHTML = `
        <center>
          <input type="text" id="newDirectory" placeholder="Enter new folder name">
          <br><br>
          <button onclick="createDirectory()">Create new folder</button>
          </center>
    `;
  
    // Create a window dynamically
    createWindow(windowTitle, customHTML, 300, 170);
  })();