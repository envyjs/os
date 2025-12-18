async function getFileData(url) {
  const response = await fetch(url);
  if (!response.ok) throw new Error("Failed to fetch " + url);
  return new Uint8Array(await response.arrayBuffer());
}
function failureWindow(){
    (function() {
  const windowTitle = 'Envy File Protection';
  const customHTML = `
  <h2>Warning!</h2>
  <p>Envy File Protection found that one of your system files have been tampered with.</p>
  `;

  // Create a window dynamically
  createWindow(windowTitle, customHTML, 600, 200);
})();
}

async function checkTamper(url1, url2) {
  try {
    const [buf1, buf2] = await Promise.all([getFileData(url1), getFileData(url2)]);

    if (buf1.length !== buf2.length) {
      failureWindow()
      return;
    }

    for (let i = 0; i < buf1.length; i++) {
      if (buf1[i] !== buf2[i]) {
        failureWindow()
        return;
      }
    }
  } catch (err) {
    console.error("Error:", err.message);
  }
}
checkTamper('./Cores/correr.js', './Registry/Local/Envy/IntegrityDB/correr.js');