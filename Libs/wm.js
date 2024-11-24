// GWM (Giovanni Window Manager)
// (C) OwnedByWuigi 2024

let topZIndex = 1; // Track the highest z-index

function createWindow(titleText, contentHTML, customMenuOptions) {
    const windowDiv = document.createElement('div');
    windowDiv.classList.add('window');

    windowDiv.originalState = {
        width: '400px',
        height: '300px',
        left: '100px',
        top: '100px'
    };

    let isMaximized = false;

    // Set initial z-index and bring to front
    windowDiv.style.zIndex = topZIndex;

    // Create window header with title and close button
    const header = document.createElement('div');
    header.classList.add('window-header');

    const title = document.createElement('span');
    title.textContent = titleText || 'Window Title';
    header.appendChild(title);

    const closeButton = document.createElement('button');
    closeButton.classList.add('close-btn');
    closeButton.addEventListener('click', () => closeWindow(windowDiv));

    header.appendChild(closeButton);
    windowDiv.appendChild(header);

    const body = document.createElement('div');
    body.classList.add('window-body');
    body.innerHTML = contentHTML || 'This is a draggable window!';
    windowDiv.appendChild(body);

    const resizeHandles = ['top-left', 'top-right', 'bottom-left', 'bottom-right', 'top', 'right', 'bottom', 'left'];
    resizeHandles.forEach(handle => {
        const resizeHandle = document.createElement('div');
        resizeHandle.classList.add('resize-handle', handle);
        windowDiv.appendChild(resizeHandle);
        makeWindowResizable(windowDiv, resizeHandle, handle);
    });

    document.getElementById('windowContainer').appendChild(windowDiv);
    setTimeout(() => windowDiv.classList.add('open'), 10);

    makeWindowDraggable(windowDiv, header);

    windowDiv.addEventListener('contextmenu', (e) => openContextMenu(e, windowDiv, customMenuOptions));

    // Double-click event for maximizing/restoring
    header.addEventListener('dblclick', () => {
        if (isMaximized) {
            windowDiv.classList.remove('maximized');
            isMaximized = false;
        } else {
            windowDiv.classList.add('maximized');
            isMaximized = true;
        }
    });

    // Bring the window to the front when the title bar is clicked
    header.addEventListener('click', () => {
        topZIndex++;
        windowDiv.style.zIndex = topZIndex;
    });
}

function closeWindow(windowDiv) {
    windowDiv.classList.remove('open');
    windowDiv.classList.add('close');
    setTimeout(() => {
        windowDiv.remove();
    }, 400);
}

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

            const minWidth = 150;
            const minHeight = 100;

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

function openContextMenu(e, windowDiv, customMenuOptions) {
    e.preventDefault();
    const contextMenu = document.getElementById('contextMenu');
    contextMenu.innerHTML = '';
    const ul = document.createElement('ul');
    customMenuOptions.forEach(option => {
        const li = document.createElement('li');
        li.textContent = option.label;
        li.onclick = () => option.action(windowDiv);
        ul.appendChild(li);
    });
    contextMenu.appendChild(ul);
    contextMenu.style.left = `${e.pageX}px`;
    contextMenu.style.top = `${e.pageY}px`;
    contextMenu.style.display = 'block';
    document.addEventListener('click', () => {
        contextMenu.style.display = 'none';
    }, { once: true });
}

function minimizeWindow(windowDiv) {
    windowDiv.style.height = '40px';
    windowDiv.style.overflow = 'hidden';
}

function maximizeWindow(windowDiv) {
    windowDiv.classList.add('maximized');
}

console.log("[WM] Window Manager loaded");
