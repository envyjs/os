/* -*- Mode: JavaScript; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */

(function() {
  const windowTitle = 'About Envy';
  const customHTML = `
  <center>
  <img src="./Assets/brand/big.webp" height="60">
  </center>
  <h2 style="line-height: 5px;">Envy</h2>
  <p>Version 10.0.5902</p>
  <p>Copyright (C) Envy Group 2022-2027</p>
  <p>This product is licensed under the Envy Public License</p>
  `;

  // Create a window dynamically
  createWindow(windowTitle, customHTML, 350, 320);
})();