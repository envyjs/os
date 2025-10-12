(function() {
  const windowTitle = 'Envy Recovery';
  const customHTML = `
    <button style="width: 100%; height: 130px; padding-left: 20px; text-align: left;" onclick="loadScript('./UI/install.js')">
    <h2>Reinstall Envy</h2>
    <p>Install a new copy of Envy onto your device.</p>
    </button>
    <br><br>
    <button style="width: 100%; height: 130px; padding-left: 20px; text-align: left;" onclick="loadScript('./Apps/Terminal.js')">
    <h2>Terminal</h2>
    <p>Access a recovery terminal.</p>
    </button>
    <br><br>
    <button style="width: 100%; height: 130px; padding-left: 20px; text-align: left;" onclick="loadScript('./Apps/Terminal.js')">
    <h2>Recover from backup</h2>
    <p>If you have a backup you want to restore from.</p>
    </button>
    <br><br>
    <button style="width: 100%; height: 130px; padding-left: 20px; text-align: left;" onclick="location.reload();">
    <h2>Restart</h2>
    <p>Attempt to reboot Envy.</p>
    </button>
  `;

  // Create a window dynamically
  createWindow(windowTitle, customHTML, 400, 650);
})();

document.body.style.backgroundColor = "#0f0f0fff";
document.querySelectorAll('.window-minimize-btn').forEach(el => el.style.display = 'none');
document.querySelectorAll('.window-maximize-btn').forEach(el => el.style.display = 'none');
document.querySelectorAll('.window-close-btn').forEach(el => el.style.display = 'none');