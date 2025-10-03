// env.js
// This file sets up the environment by loading different scripts based on the specified type.

const type = 'client'; // Change this to 'client', 'install', 'server', 'serverwc' or 'recovery' as needed

// Function to load and execute a script depending on the type

if (type === 'client') {
    loadScript('./UI/lock.js');
}
if (type === 'install') {
    loadScript('./UI/install.js');
}
if (type === 'server') {
    loadScript('./UI/server.js');
}
if (type === 'serverwc') {
    loadScript('./UI/desktop.js');
    loadScript('./UI/server.js');
}
if (type === 'recovery') {
    loadScript('./UI/recovery.js');
}