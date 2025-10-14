(function() {
  const windowTitle = ' ';
  const customHTML = `
  <h2 data-i18n="envy.oobe.title"></h2>
    <p data-i18n="envy.oobe.description"></p>
  <label for="lang">Language: </label>
  <select name="lang" id="lang" onchange="setLanguage(this.value)">
    <option value="en">English</option>
    <option value="es">Spanish</option>
  </select>
  `;

  // Create a window dynamically
  createWindow(windowTitle, customHTML, 600, 450);
})();
document.querySelectorAll('.window-minimize-btn').forEach(el => el.style.display = 'none');
document.querySelectorAll('.window-maximize-btn').forEach(el => el.style.display = 'none');
document.body.style.backgroundImage = "url('./Assets/wallpaper/stable/wp.jpg')";
function setEnglish() {
  setLanguage("en");
}

setTimeout(setEnglish, 100);