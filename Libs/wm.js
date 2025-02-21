let topZIndex = 1; // Track the highest z-index
function createWindow(titleText, contentHTML, width = 400, height = 300) {
    const windowId = `window-${Date.now()}`;
    const windowDiv = document.createElement('div');
    windowDiv.classList.add('window');
    windowDiv.id = windowId;

    // Set initial z-index and bring to front
    windowDiv.style.zIndex = topZIndex;

    // Set default size
    windowDiv.style.width = `${width}px`;
    windowDiv.style.height = `${height}px`;

    // Position randomly within viewport
    windowDiv.style.left = `20px`;
    windowDiv.style.top = `20px`;

    // Header with title and controls
    const header = document.createElement('div');
    header.classList.add('window-header');

    header.addEventListener('click', () => {
        topZIndex++;
        windowDiv.style.zIndex = topZIndex;
    });

    const title = document.createElement('span');
    title.textContent = titleText || 'New Window';
    header.appendChild(title);

    // Header buttons container
    const headerButtons = document.createElement('div');
    headerButtons.classList.add('window-header-buttons');

    // Minimize Button
    const minimizeButton = document.createElement('button');
    minimizeButton.classList.add('window-minimize-btn');
    minimizeButton.textContent = '';
    minimizeButton.addEventListener('click', () => minimizeWindow(windowId));

    // Maximize Button
    const maximizeButton = document.createElement('button');
    maximizeButton.classList.add('window-maximize-btn');
    maximizeButton.textContent = '';
    maximizeButton.addEventListener('click', () => maximizeWindow(windowId));

    // Close Button
    const closeButton = document.createElement('button');
    closeButton.classList.add('window-close-btn');
    closeButton.textContent = '';
    closeButton.addEventListener('click', () => closeWindow(windowId));

    // Append buttons to header
    headerButtons.appendChild(minimizeButton);
    headerButtons.appendChild(maximizeButton);
    headerButtons.appendChild(closeButton);
    header.appendChild(headerButtons);
    windowDiv.appendChild(header);

    // Body for content
    const body = document.createElement('div');
    body.classList.add('window-body');
    body.innerHTML = contentHTML || 'Default Content';
    windowDiv.appendChild(body);

    // Create resizing handles
    const resizeHandles = ['top-left', 'top-right', 'bottom-left', 'bottom-right', 'top', 'right', 'bottom', 'left'];
    resizeHandles.forEach(handle => {
        const resizeHandle = document.createElement('div');
        resizeHandle.classList.add('resize-handle', handle);
        windowDiv.appendChild(resizeHandle);
        makeWindowResizable(windowDiv, resizeHandle, handle);
    });

    // Add to the window container
    const windowContainer = document.getElementById('userland');
    if (!windowContainer) {
        console.error('Error: "userland" element is missing.');
        return;
    }
    windowContainer.appendChild(windowDiv);

    // Add to taskbar
    addTaskbarButton(windowId, titleText);

    // Ensure window is visible upon launch and animate opening
    windowDiv.classList.add('open');

    // Make the window draggable
    makeWindowDraggable(windowDiv, header);
}

// Close Window with animation
function closeWindow(windowId) {
    const windowDiv = document.getElementById(windowId);
    if (windowDiv) {
        windowDiv.classList.remove('open'); // Trigger closing animation
        windowDiv.classList.add('close'); // Apply close class for fade and shrink

        // Wait for the animation to complete, then remove the window
        setTimeout(() => {
            windowDiv.remove();
            removeTaskbarButton(windowId);
        }, 400); // 400ms corresponds to the duration of the animation
    }
}

// Minimize Window
function minimizeWindow(windowId) {
    const windowDiv = document.getElementById(windowId);
    if (windowDiv) {
        windowDiv.classList.toggle('minimized');
    }
}

// Maximize Window
function maximizeWindow(windowId) {
    const windowDiv = document.getElementById(windowId);
    if (windowDiv) {
        windowDiv.classList.toggle('maximized');
    }
}

// Add Taskbar Button
function addTaskbarButton(windowId, title) {
    const taskbar = document.getElementById("taskbarApps");
    if (!taskbar) {
        console.error("Taskbar not found!");
        return;
    }

    const button = document.createElement("button");
    button.textContent = title;
    button.classList.add("taskbar-button");
    button.dataset.window = windowId;

    button.addEventListener("click", () => {
        const windowDiv = document.getElementById(windowId);
        if (windowDiv) {
            windowDiv.classList.toggle('minimized');
        }
    });

    taskbar.appendChild(button);
}

// Remove Taskbar Button
function removeTaskbarButton(windowId) {
    const taskbar = document.getElementById("taskbarApps");
    if (taskbar) {
        const button = taskbar.querySelector(`button[data-window="${windowId}"]`);
        if (button) {
            button.remove();
        }
    }
}

// Make Window Draggable
function makeWindowDraggable(window, header) {
    let isDragging = false;
    let offsetX = 0;
    let offsetY = 0;

    header.addEventListener('mousedown', (e) => {
        isDragging = true;
        offsetX = e.clientX - window.offsetLeft;
        offsetY = e.clientY - window.offsetTop;
        document.addEventListener('mousemove', onMouseMove);
        document.addEventListener('mouseup', onMouseUp);
    });

    function onMouseMove(e) {
        if (isDragging) {
            window.style.left = `${e.clientX - offsetX}px`;
            window.style.top = `${e.clientY - offsetY}px`;
        }
    }

    function onMouseUp() {
        isDragging = false;
        document.removeEventListener('mousemove', onMouseMove);
        document.removeEventListener('mouseup', onMouseUp);
    }
}

// Make Window Resizable
function makeWindowResizable(window, handle, direction) {
    let isResizing = false;
    let lastX = 0;
    let lastY = 0;

    handle.addEventListener('mousedown', (e) => {
        isResizing = true;
        lastX = e.clientX;
        lastY = e.clientY;
        document.addEventListener('mousemove', onResize);
        document.addEventListener('mouseup', stopResize);
        e.preventDefault();
    });

    function onResize(e) {
        if (isResizing) {
            const deltaX = e.clientX - lastX;
            const deltaY = e.clientY - lastY;

            const minWidth = 200;
            const minHeight = 150;

            if (direction.includes('right')) {
                window.style.width = `${Math.max(window.offsetWidth + deltaX, minWidth)}px`;
            }
            if (direction.includes('bottom')) {
                window.style.height = `${Math.max(window.offsetHeight + deltaY, minHeight)}px`;
            }
            if (direction.includes('left')) {
                window.style.width = `${Math.max(window.offsetWidth - deltaX, minWidth)}px`;
                window.style.left = `${window.offsetLeft + deltaX}px`;
            }
            if (direction.includes('top')) {
                window.style.height = `${Math.max(window.offsetHeight - deltaY, minHeight)}px`;
                window.style.top = `${window.offsetTop + deltaY}px`;
            }

            lastX = e.clientX;
            lastY = e.clientY;
        }
    }

    function stopResize() {
        isResizing = false;
        document.removeEventListener('mousemove', onResize);
        document.removeEventListener('mouseup', stopResize);
    }
}

// Apply CSS for scrollbars in the style section
const style = document.createElement('style');
style.textContent = `
    .window-body {
        overflow: auto;  /* Enables scrollbars when content overflows */
        max-height: calc(100% - 40px); /* Ensures body doesn't exceed window height */
    }
`;
document.head.appendChild(style);