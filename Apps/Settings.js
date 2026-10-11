/* -*- Mode: JavaScript; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
(function() {
  const windowTitle = 'Settings';
  const customHTML = `
    <div class="settings-app">
      <aside class="settings-sidebar">
        <button class="settings-nav active" data-page="display">Display</button>
        <button class="settings-nav" data-page="defaults">Default apps</button>
      </aside>
      <main class="settings-main">
        <section class="settings-page" id="settings-display">
          <h2>Display</h2>
          <p class="settings-description">Choose a screen resolution supported by this display adapter.</p>
          <div class="settings-card">
            <div><strong>Screen resolution</strong><small>Changes apply immediately.</small></div>
            <select id="settingsResolution" aria-label="Screen resolution"></select>
            <button id="settingsResolutionApply" class="settings-primary">Apply</button>
          </div>
          <p id="settingsDisplayStatus" class="settings-status" role="status"></p>
        </section>
        <section class="settings-page" id="settings-defaults" style="display:none">
          <h2>Default apps</h2>
          <p class="settings-description">Choose which app opens each kind of file from Filer.</p>
          <div class="settings-card settings-default-row"><div><strong>Text files</strong><small>text/*</small></div><select id="defaultText"></select></div>
          <div class="settings-card settings-default-row"><div><strong>Images</strong><small>image/*</small></div><select id="defaultImage"></select></div>
          <div class="settings-card settings-default-row"><div><strong>Audio</strong><small>audio/*</small></div><select id="defaultAudio"></select></div>
          <div class="settings-card settings-default-row"><div><strong>Video</strong><small>video/*</small></div><select id="defaultVideo"></select></div>
          <p id="settingsDefaultStatus" class="settings-status" role="status">Choices are kept for this session.</p>
        </section>
      </main>
    </div>
  `;
  const settingsWindow = createWindow(windowTitle, customHTML, 720, 470);
  settingsWindow.classList.add('settings-window');
  const displayApi = window.kernel && window.kernel.display;
  const settingsApi = window.kernel && window.kernel.settings;

  function addOption(select, value, label) {
    const option = document.createElement('option');
    option.setAttribute('value', value);
    option.textContent = label;
    select.appendChild(option);
  }

  function setupResolution() {
    const select = settingsWindow.querySelector('#settingsResolution');
    const status = settingsWindow.querySelector('#settingsDisplayStatus');
    const apply = settingsWindow.querySelector('#settingsResolutionApply');
    if (!select || !displayApi) {
      if (status) status.textContent = 'Runtime display control is only available in NuKernel.';
      if (select) select.disabled = true;
      if (apply) apply.disabled = true;
      return;
    }
    const modes = displayApi.availableResolutions();
    const current = displayApi.currentResolution();
    for (let i = 0; i < modes.length; i++) {
      const mode = modes[i];
      const value = mode.width + 'x' + mode.height;
      addOption(select, value, mode.width + ' x ' + mode.height);
      if (mode.width === current.width && mode.height === current.height) select.value = value;
    }
    if (!modes.length) {
      if (status) status.textContent = 'No display modes were reported by the adapter.';
      select.disabled = true;
      apply.disabled = true;
      return;
    }
    apply.addEventListener('click', function() {
      const parts = select.value.split('x');
      const ok = displayApi.setResolution(Number(parts[0]), Number(parts[1]));
      if (status) status.textContent = ok ? 'Resolution changed to ' + select.value + '.' : 'The adapter could not apply that resolution.';
    });
    if (status) status.textContent = modes.length + ' supported display modes.';
  }

  function setupDefault(selectId, mime, choices) {
    const select = settingsWindow.querySelector('#' + selectId);
    if (!select) return;
    for (let i = 0; i < choices.length; i++) addOption(select, choices[i][0], choices[i][1]);
    const saved = settingsApi && settingsApi.getDefaultApp(mime);
    select.value = saved || 'builtin';
    select.addEventListener('change', function() {
      if (!settingsApi) return;
      const status = settingsWindow.querySelector('#settingsDefaultStatus');
      const ok = settingsApi.setDefaultApp(mime, select.value);
      if (status) status.textContent = ok ? 'Default app updated for ' + mime + '.' : 'Could not save this default.';
    });
  }

  setupResolution();
  setupDefault('defaultText', 'text/*', [['builtin', 'Envy file viewer'], ['./Apps/Lexis.js', 'Lexis']]);
  setupDefault('defaultImage', 'image/*', [['builtin', 'Envy image viewer'], ['./Apps/Media.js', 'Media']]);
  setupDefault('defaultAudio', 'audio/*', [['builtin', 'Envy file viewer'], ['./Apps/Media.js', 'Media']]);
  setupDefault('defaultVideo', 'video/*', [['builtin', 'Envy file viewer'], ['./Apps/Media.js', 'Media']]);

  const navButtons = settingsWindow.querySelectorAll('.settings-nav');
  for (let i = 0; i < navButtons.length; i++) navButtons[i].addEventListener('click', function() {
    const pageName = navButtons[i].getAttribute('data-page');
    const pages = settingsWindow.querySelectorAll('.settings-page');
    for (let p = 0; p < pages.length; p++) pages[p].style.display = pages[p].id === 'settings-' + pageName ? 'block' : 'none';
    for (let n = 0; n < navButtons.length; n++) navButtons[n].classList.toggle('active', navButtons[n] === this);
  });
})();
