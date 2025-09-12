(function() {
  const windowTitle = 'Filer';
  const customHTML = `
  <span style="gap: 20px; display: flex;">
  <a onclick="loadScript('./API/Filer.CreateDir.js');" class="filericon"><img src="./Assets/filer/folder-plus.svg"></a>
  <a onclick="loadScript('./API/Filer.CreateFile.js');" class="filericon"><img src="./Assets/filer/file.svg"></a>
  <input class="filertext" style="width: calc(100% - 60px)" "type="text"> 
  </span>
        <div class="file-list" id="fileList">
            <div id="filesContainer">
            </div>
        </div>
  `;

  loadFiles();

  // Create a window dynamically
  createWindow(windowTitle, customHTML, 650, 400);
})();