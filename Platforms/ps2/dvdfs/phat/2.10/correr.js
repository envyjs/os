// Experimental port of the Correr core to the Athena platform (PS2)

// get screen settings and enable depth buffer
const canvas = Screen.getMode();
canvas.zbuffering = true;
canvas.psmz = Z16S;
Screen.setMode(canvas);

// enable frame counter and vsync
Screen.setFrameCounter(true);
Screen.setVSync(false);

function loadScript(url, callback) {
    fetch(url)
        .then(response => {
            if (!response.ok) {
                throw new Error(`[CORE] Failed to load ${url} (status ${response.status})`);
            }
            return response.text();
        })
        .then(code => {
            console.log(`[CORE] Successfully loaded ${url}`);
            eval(code); // executes the fetched script
            if (callback) callback();
        })
        .catch(err => {
            console.error(`[CORE] Error loading ${url}:`, err);
        });
}

loadScript('wm.js');
loadScript('desktop.js');