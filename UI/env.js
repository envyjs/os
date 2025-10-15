// env.js
// This file sets up the environment by loading different scripts based on the specified type.

const type = 'desktop'; // Change this to 'desktop', 'go', 'live', 'nogui', 'install', 'server', 'serverwc' or 'recovery' as needed

// Function to load and execute a script depending on the type

// Desktop types
if (type === 'desktop') {
    loadScript('./Libs/lang.js')
    const link = document.createElement('link');
    link.rel = 'stylesheet';
    link.href = './Styles/cherrytree.css';
    document.head.appendChild(link);
    const link2 = document.createElement('link');
    link2.rel = 'stylesheet';
    link2.href = './Styles/ewinbox.css';
    document.head.appendChild(link2);
    const link3 = document.createElement('link');
    link3.rel = 'stylesheet';
    link3.href = './Styles/base.css';
    document.head.appendChild(link3);
    loadScript('./Libs/html.js')
    loadScript('./Libs/ewinboxloader.js')
    loadScript('./Libs/jsmediatags.js');
    loadScript('./Libs/noti.js')
    loadScript('./Libs/winbox.js')
    loadScript('./Libs/gamepad.correr.js')
    loadScript('./Libs/jquery.js')
    loadScript('./Libs/cheerpx.js')
    loadScript('./Libs/crypt.js')
    loadScript('./API/Linux.Initialize.js')
    loadScript('./UI/lock.js');
    loadScript('./Apps/Welcome.js');
}
if (type === 'live') {
    loadScript('./Libs/lang.js')
    const link = document.createElement('link');
    link.rel = 'stylesheet';
    link.href = './Styles/cherrytree.css';
    document.head.appendChild(link);
    const link2 = document.createElement('link');
    link2.rel = 'stylesheet';
    link2.href = './Styles/ewinbox.css';
    const link3 = document.createElement('link');
    link3.rel = 'stylesheet';
    link3.href = './Styles/base.css';
    document.head.appendChild(link3);
    loadScript('./Libs/html.js')
    loadScript('./Libs/ewinboxloader.js')
    loadScript('./Libs/noti.js')
    loadScript('./Libs/winbox.js')
    loadScript('./Libs/gamepad.correr.js')
    loadScript('./Libs/jquery.js')
    loadScript('./Libs/cheerpx.js')
    loadScript('./Libs/crypt.js')
    loadScript('./UI/desktop.js');
    loadScript('./API/System.LiveAlert.js');
}
if (type === 'serverwc') {
    loadScript('./Libs/lang.js')
    const link = document.createElement('link');
    link.rel = 'stylesheet';
    link.href = './Styles/cherrytree.css';
    document.head.appendChild(link);
    const link2 = document.createElement('link');
    link2.rel = 'stylesheet';
    link2.href = './Styles/ewinbox.css';
    document.head.appendChild(link2);
    const link3 = document.createElement('link');
    link3.rel = 'stylesheet';
    link3.href = './Styles/base.css';
    document.head.appendChild(link3);
    loadScript('./Libs/html.js')
    loadScript('./Libs/ewinboxloader.js')
    loadScript('./Libs/noti.js')
    loadScript('./Libs/winbox.js')
    loadScript('./Libs/gamepad.correr.js')
    loadScript('./Libs/jquery.js')
    loadScript('./Libs/cheerpx.js')
    loadScript('./Libs/crypt.js')
    loadScript('./UI/desktop.js');
    loadScript('./UI/server.js');
}
// Alternative shell types
if (type === 'go') {
    const link3 = document.createElement('link');
    link3.rel = 'stylesheet';
    link3.href = './Styles/base.css';
    document.head.appendChild(link3);
    loadScript('./Libs/html.js')
    loadScript('./Libs/noti.js')
    loadScript('./Libs/winbox.js')
    loadScript('./Libs/osk.js')
    loadScript('./Libs/gamepad.correr.js')
    loadScript('./Libs/jquery.js')
    loadScript('./Libs/jsmediatags.js');
    loadScript('./Libs/crypt.js')
    loadScript('./UI/go.js')
}
if (type === 'install') {
    const link3 = document.createElement('link');
    link3.rel = 'stylesheet';
    link3.href = './Styles/base.css';
    document.head.appendChild(link3);
    loadScript('./UI/install.js');
}
if (type === 'server') {
    const link3 = document.createElement('link');
    link3.rel = 'stylesheet';
    link3.href = './Styles/server.css';
    document.head.appendChild(link3);
    loadScript('./Libs/mue.js');
    loadScript('./UI/server.js');
}
if (type === 'recovery') {
    const link3 = document.createElement('link');
    link3.rel = 'stylesheet';
    link3.href = './Styles/base.css';
    document.head.appendChild(link3);
    loadScript('./UI/recovery.js');
}