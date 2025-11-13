/* -*- Mode: JavaScript; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
(function() {
  const windowTitle = 'Example App';
  const customHTML = `
    <h2>Example App</h2>
    <p>This is the example app.</p>
    <p>مرحبا فرو</p>
  `;

  // Create a window dynamically
  createWindow(windowTitle, customHTML, 600, 350);
})();

notifier.create('Hello from the Example App', 'info', 5000);