/* -*- Mode: JavaScript; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
(function() {
  const windowTitle = 'Filer';
  const customHTML = `
    <div class="filer-toolbar">
      <input id="filerPath" class="filertext" type="text" value="/">
      <button class="filer-open-button" onclick="filerGo()">Open</button>
    </div>
    <div id="filerStatus" role="status">Loading filesystem...</div>
    <div class="file-list" id="fileList"><div id="filesContainer"></div></div>
  `;

  createWindow(windowTitle, customHTML, 650, 400);
  var currentPath = '/';
  var previewId = 0;
  var mimeTypes = {
    txt: 'text/plain', html: 'text/html', htm: 'text/html', css: 'text/css',
    js: 'application/javascript', json: 'application/json', png: 'image/png',
    jpg: 'image/jpeg', jpeg: 'image/jpeg', gif: 'image/gif', svg: 'image/svg+xml'
  };
  if (window.filesystem && typeof window.filesystem.readText === 'function') {
    var mimeFile = window.filesystem.readText('/Libs/mime.json');
    if (mimeFile) {
      try {
        var loadedMimeTypes = JSON.parse(mimeFile);
        for (var extension in loadedMimeTypes) {
          if (Object.prototype.hasOwnProperty.call(loadedMimeTypes, extension)) {
            mimeTypes[extension.toLowerCase()] = loadedMimeTypes[extension];
          }
        }
      } catch (error) {
        console.warn('Filer could not read the MIME type list.', error);
      }
    }
  }

  function getMimeType(name) {
    var baseName = name.substring(name.lastIndexOf('/') + 1);
    var dot = baseName.lastIndexOf('.');
    var extension = dot >= 0 ? baseName.substring(dot + 1).toLowerCase() : '';
    return mimeTypes[extension] || 'application/octet-stream';
  }

  function showPreview(name, mimeType, content, imagePath) {
    var id = 'filerPreview' + previewId++;
    var previewWindow = createWindow(name, '<div class="filer-preview" id="' + id + '"></div>', 650, 450);
    var preview = previewWindow && previewWindow.querySelector('#' + id);
    if (!preview) return;
    var typeLabel = document.createElement('div');
    typeLabel.className = 'filer-preview-type';
    typeLabel.textContent = mimeType;
    preview.appendChild(typeLabel);
    if (imagePath) {
      var image = document.createElement('img');
      image.className = 'filer-preview-image';
      image.setAttribute('src', './' + imagePath.replace(/^\/+/, ''));
      image.setAttribute('alt', name);
      preview.appendChild(image);
    } else if (content !== null) {
      var text = document.createElement('pre');
      text.className = 'filer-preview-text';
      text.textContent = content;
      preview.appendChild(text);
    } else {
      var message = document.createElement('p');
      message.textContent = 'No preview is available for this file type.';
      preview.appendChild(message);
    }
  }

  window.filerOpenFile = function(path, name) {
    var mimeType = getMimeType(name || path);
    var textType = mimeType.indexOf('text/') === 0 || mimeType === 'application/json' ||
      mimeType === 'application/javascript' || mimeType === 'application/xml' ||
      mimeType === 'application/rtf';
    var content = textType && window.filesystem && typeof window.filesystem.readText === 'function'
      ? window.filesystem.readText(path) : null;
    var settings = window.kernel && window.kernel.settings;
    var handler = settings && settings.getDefaultApp(mimeType);
    if (!handler && settings && mimeType.indexOf('/') >= 0) {
      handler = settings.getDefaultApp(mimeType.substring(0, mimeType.indexOf('/')) + '/*');
    }
    if (handler && handler !== 'builtin' && typeof loadScript === 'function') {
      window.openedFile = { path: path, name: name || path, mimeType: mimeType, content: content };
      loadScript(handler);
      return;
    }
    if (mimeType.indexOf('image/') === 0) {
      showPreview(name || path, mimeType, null, path);
      return;
    }
    showPreview(name || path, mimeType, content, null);
  };

  window.filerGo = function(path) {
    var field = document.getElementById('filerPath');
    window.loadFiles(path || (field ? field.value : currentPath));
  };
  window.loadFiles = function(path) {
    var files = document.getElementById('filesContainer');
    var status = document.getElementById('filerStatus');
    if (!window.filesystem || typeof window.filesystem.list !== 'function') {
      if (status) status.textContent = 'Kernel filesystem API is unavailable.';
      return;
    }
    currentPath = path || '/';
    var entries = window.filesystem.list(currentPath);
    if (!entries) {
      if (status) status.textContent = 'Cannot open ' + currentPath;
      if (files) files.innerHTML = '';
      return;
    }
    if (files) files.innerHTML = '';
    if (currentPath !== '/') {
      var parent = currentPath.replace(/\/$/, '').replace(/\/[^/]*$/, '') || '/';
      var up = document.createElement('button');
      up.className = 'filer-entry-button filer-parent-button';
      up.textContent = 'Up one folder';
      up.addEventListener('click', function() { window.loadFiles(parent); });
      files.appendChild(up);
    }
    for (var i = 0; i < entries.length; i++) {
      var entry = entries[i];
      var row = document.createElement('div');
      var itemButton = document.createElement('button');
      itemButton.className = 'filer-entry-button filer-item-button';
      var label = document.createElement('span');
      label.className = 'filer-entry-name';
      label.textContent = entry.name;
      itemButton.appendChild(label);
      var type = document.createElement('small');
      type.className = 'filer-entry-type';
      type.textContent = entry.directory ? 'Folder' : getMimeType(entry.name);
      itemButton.appendChild(type);
      var size = document.createElement('small');
      size.className = 'filer-entry-size';
      size.textContent = entry.directory ? '-' : entry.size + ' bytes';
      itemButton.appendChild(size);
      if (entry.directory) {
        itemButton.addEventListener('click', function(nextPath) {
          return function() { window.loadFiles(nextPath); };
        }(entry.path));
      } else {
        itemButton.addEventListener('click', function(filePath, fileName) {
          return function() { window.filerOpenFile(filePath, fileName); };
        }(entry.path, entry.name));
      }
      row.appendChild(itemButton);
      row.className = 'filer-entry';
      files.appendChild(row);
    }
    if (files && !entries.length) files.textContent = '(empty directory)';
    if (status) status.textContent = window.filesystem.name + ' | ' + currentPath + ' | ' + entries.length + ' items';
    var field = document.getElementById('filerPath');
    if (field) field.value = currentPath;
  };
  window.loadFiles('/');
})();
