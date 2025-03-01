(function() {
  const windowTitle = 'Filer';
  const customHTML = `
  <span style="gap: 20px; display: flex;">
  <a onclick="loadScript('./API/Filer.CreateDir.js');"><img src="./Assets/filer/folder-plus.svg"></a>
  <a onclick="loadScript('./API/Filer.CreateFile.js');"><img src="./Assets/filer/file.svg"></a>
  </span>
        <div class="file-list" id="fileList">
        <br>
            <div id="filesContainer">
            </div>
        </div>
  `;

  loadFiles();

  // Create a window dynamically
  createWindow(windowTitle, customHTML, 600, 350);
})();