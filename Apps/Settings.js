/* -*- Mode: JavaScript; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
(function() {
  const windowTitle = 'Settings';
  const customHTML = `
  <div style="display: flex;">
    <div style="width: 30%">
    <button style="min-width: 87%; height: 55px; text-align: left; line-height: 5px; display: flex;">
      <div style="width: 25%; margin-top: 12px">
        <img src="./Assets/settings/brush.svg">
      </div>
      <div style="width: 75%;">
        <h4>Home</h4>
      </div>
    </button>
    <br>
    <button style="min-width: 87%; height: 55px; text-align: left; line-height: 5px; display: flex;">
      <div style="width: 25%; margin-top: 12px">
        <img src="./Assets/settings/brush.svg">
      </div>
      <div style="width: 75%;">
        <h4>Appearence</h4>
      </div>
    </button>
    <br>
    <button style="min-width: 87%; height: 55px; text-align: left; line-height: 5px; display: flex;">
      <div style="width: 25%; margin-top: 12px">
        <img src="./Assets/settings/brush.svg">
      </div>
      <div style="width: 75%;">
        <h4>Network</h4>
      </div>
    </button>
    </div>
    <div style="width: 70%">
      <h2>Appearence</h2>
      <button onclick="loadScript('/Libs/spotlight.js')">Envy Spotlight</button>
    </div>
  `;
  createWindow(windowTitle, customHTML, 600, 350);
})();