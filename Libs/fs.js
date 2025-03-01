class VirtualFileSystem {
            constructor(dbName = 'VFS', storeName = 'files') {
                this.dbName = dbName;
                this.storeName = storeName;
                this.db = null;
                this.mimeTypes = {}; // Will be loaded from mime.json
                this.currentDir = '/'; // Keeps track of the current directory
            }

            async init() {
                await this.loadMimeTypes();
                return new Promise((resolve, reject) => {
                    const request = indexedDB.open(this.dbName, 1);

                    request.onupgradeneeded = (event) => {
                        const db = event.target.result;
                        if (!db.objectStoreNames.contains(this.storeName)) {
                            db.createObjectStore(this.storeName, { keyPath: 'name' });
                        }
                    };

                    request.onsuccess = (event) => {
                        this.db = event.target.result;
                        resolve();
                    };

                    request.onerror = (event) => reject(event.target.error);
                });
            }

            async loadMimeTypes() {
                try {
                    const response = await fetch('./Libs/mime.json'); // Load from external JSON
                    this.mimeTypes = await response.json();
                } catch (error) {
                    console.error("Failed to load MIME types:", error);
                }
            }

            getMimeType(fileName) {
                const ext = fileName.split('.').pop().toLowerCase();
                return this.mimeTypes[ext] || 'application/octet-stream';
            }

            async writeFile(name, content) {
                const mimeType = this.getMimeType(name);
                return new Promise((resolve, reject) => {
                    const transaction = this.db.transaction([this.storeName], 'readwrite');
                    const store = transaction.objectStore(this.storeName);
                    const request = store.put({ name, content, mimeType });

                    request.onsuccess = () => resolve();
                    request.onerror = (event) => reject(event.target.error);
                });
            }

            async readFile(name) {
                return new Promise((resolve, reject) => {
                    const transaction = this.db.transaction([this.storeName], 'readonly');
                    const store = transaction.objectStore(this.storeName);
                    const request = store.get(name);

                    request.onsuccess = () => {
                        const file = request.result;
                        if (file) {
                            resolve(file);
                        } else {
                            resolve(null);
                        }
                    };

                    request.onerror = (event) => reject(event.target.error);
                });
            }

            async deleteFile(name) {
                return new Promise((resolve, reject) => {
                    const transaction = this.db.transaction([this.storeName], 'readwrite');
                    const store = transaction.objectStore(this.storeName);
                    const request = store.delete(name);

                    request.onsuccess = () => resolve();
                    request.onerror = (event) => reject(event.target.error);
                });
            }

            async listFiles() {
                return new Promise((resolve, reject) => {
                    const transaction = this.db.transaction([this.storeName], 'readonly');
                    const store = transaction.objectStore(this.storeName);
                    const request = store.getAll();

                    request.onsuccess = () => resolve(request.result);
                    request.onerror = (event) => reject(event.target.error);
                });
            }

            // Change the current directory and reload the file list
            changeDirectory(newDir) {
                this.currentDir = newDir;
                document.getElementById('currentDirectory').innerText = this.currentDir;
                loadFiles(); // Reload files after changing directory
            }

            // Add a file to the current directory
            async saveFileToCurrentDirectory(fileName, content) {
                const filePath = this.currentDir + fileName;
                await this.writeFile(filePath, content);
            }

            // Create a new directory (by just saving a placeholder file or directory)
            async createDirectory(dirName) {
                const dirPath = this.currentDir + dirName + '/';  // Append slash to indicate it's a directory
                await this.writeFile(dirPath, ''); // Store an empty file as a directory placeholder
            }
        }

        // Initialize the VFS
        const vfs = new VirtualFileSystem();

        async function initVFS() {
            await vfs.init();
            loadFiles(); // Load files after VFS initialization
        }

        // Save a file to the current directory
        async function saveFile() {
            const fileName = document.getElementById('fileName').value.trim();
            const fileContent = document.getElementById('fileContent').value.trim();

            if (!fileName || !fileContent) {
                notifier.create('Please provide both file name and content.', 'error', 5000);
                return;
            }

            await vfs.saveFileToCurrentDirectory(fileName, fileContent);
            alert('File saved successfully!');
            loadFiles(); // Refresh file list
        }

        // Create a new directory
        async function createDirectory() {
            const newDir = document.getElementById('newDirectory').value.trim();

            if (!newDir) {
                notifier.create('Please provide a folder name.', 'error', 5000);
                return;
            }

            await vfs.createDirectory(newDir);
            alert('Directory created successfully!');
            loadFiles(); // Refresh file list
        }

        // Load and display stored files
        async function loadFiles() {
            const files = await vfs.listFiles();
            const filesContainer = document.getElementById('filesContainer');
            filesContainer.innerHTML = ''; // Clear existing list

            if (files.length === 0) {
                filesContainer.innerHTML = "<p>No files stored yet.</p>";
                return;
            }

            files.forEach(file => {
                // Only show files in the current directory
                if (file.name.startsWith(vfs.currentDir)) {
                    const div = document.createElement('div');
                    div.classList.add('file-item');

                    const fileName = document.createElement('button');
                    fileName.textContent = file.name.replace(vfs.currentDir, '');
                    fileName.onclick = () => openFile(file.name);

                    const deleteButton = document.createElement('button');
                    deleteButton.textContent = 'Delete';
                    deleteButton.onclick = () => deleteFile(file.name);

                    div.appendChild(fileName);
                    div.appendChild(deleteButton);
                    filesContainer.appendChild(div);
                }
            });
        }

        // Open a file and display its content or execute it if it's a JS file
async function openFile(fileName) {
    const file = await vfs.readFile(fileName);

    if (file) {
        // Check if the file is a .js file
        if (fileName.endsWith('.js')) {
            // If it's a .js file, execute it
            executeJSFile(file.content);
        } else {
            // If it's any other file, display its content for editing
            document.getElementById('fileName').value = file.name.replace(vfs.currentDir, '');
            document.getElementById('fileContent').value = file.content;
        }
    } else if (fileName.endsWith('/')) {
        // If it's a directory (ends with '/'), navigate to it
        vfs.changeDirectory(fileName);
    } else {
        alert('File not found.');
    }
}

// Execute a JavaScript file by creating a Blob and loading it
function executeJSFile(scriptContent) {
    const blob = new Blob([scriptContent], { type: 'application/javascript' });
    const url = URL.createObjectURL(blob);

    if (typeof loadScript === "function") {
        loadScript(url, () => {
            URL.revokeObjectURL(url);  // Clean up Blob URL after script execution
        });
    } else {
        console.warn("loadScript function is not defined.");
    }
}


        // Delete a file from IndexedDB
        async function deleteFile(fileName) {
            await vfs.deleteFile(fileName);
            alert('File deleted successfully!');
            loadFiles(); // Refresh file list
        }

        // Initialize VFS when the page loads
        window.onload = initVFS;