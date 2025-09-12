
(function() {
    const windowTitle = 'Create new file';
const customHTML = `<input type="text" id="fileName" placeholder="Enter file name">
<textarea id="fileContent" placeholder="Enter file content" rows="5"></textarea>
<button onclick="saveFile()">💾 Save File</button>`;
 // Create a window dynamically
    createWindow(windowTitle, customHTML, 300, 170);
  })();