// Envy Correr custom core
// (c) OwnedByWuigi, Envy Group 2024-2027

console.log(
    "%cBoo!\n%cIf someone told you to copypaste something here, there's a 11/10 chance you don't know what you're doing. \n \nPasting any code into this console can expose your private account information and/or files to external sources. Use this console at your own risk.%c\n\nIn the rare case that you %cdo %cknow what you're doing, please contribute to this project kthxbye :3 \n\nhttps://github.com/envyjs/os",
    "color: magenta;font-size: 78px",
    "color: auto;font-size: large;",
    "color: auto;font-size:1.2rem",
    "color: auto;font-style:italic;font-size:1.2rem",
    "color: auto;font-size:1.2rem"
);

const SysInfo = {
    string: "Envy 10 2027 Update",
    version: "10.0.4523",
    extra: "Alpha",
    codename: "Astatine",
    apilevel: 3,
};

console.log(SysInfo.string, SysInfo.version, SysInfo.extra);

function loadScript(url, callback) {
    var script = document.createElement('script');
    script.type = 'text/javascript';
    script.src = url;
    script.onload = function() {
        console.log(`[CORE] Successfuly loaded ${url}`);
        if (callback) {
            callback();
        }
    };
    script.onerror = function() {
        console.error(`[CORE] Error loading ${url}`);
    };
    document.head.appendChild(script);
}
loadScript('./Libs/wm.correr.js')
loadScript('./Libs/vfs.correr.js')
loadScript('./Libs/zip.js')
console.log("[CORE] Correr core loaded");

fetch('./UI/env.js', { method: 'HEAD' })
    .then(response => {
        if (response.ok) {
            loadScript('./UI/env.js');
        } else {
            loadScript('./UI/nogui.js');
        }
    })
    .catch(() => {
        loadScript('./UI/nogui.js');
    });