(function() {
  const windowTitle = 'Install Envy';
  const customHTML = `
    <div class="installer" style="display: grid !important; grid-template-columns: 24% minmax(0, 1fr); height: 370px; gap: 24px; box-sizing: border-box;">
      <nav class="installer-nav" aria-label="Installation steps" style="min-width: 0; display: flex; flex-direction: column; gap: 8px;">
        <button type="button" data-installer-page="introduction" style="border: 0; background: transparent; cursor: pointer; padding: 4px 0; text-align: left;">Introduction</button>
        <button type="button" data-installer-page="license" style="border: 0; background: transparent; cursor: pointer; padding: 4px 0; text-align: left;">License</button>
        <button type="button" data-installer-page="options" style="border: 0; background: transparent; cursor: pointer; padding: 4px 0; text-align: left;">Install options</button>
        <button type="button" data-installer-page="install" style="border: 0; background: transparent; cursor: pointer; padding: 4px 0; text-align: left;">Install</button>
      </nav>
      <div class="verticalLine" style="min-width: 0; padding-left: 30px; position: relative; box-sizing: border-box;">
        <section data-installer-content="introduction">
          <h2>Welcome to the Envy Installer</h2>
          <p>To install Envy 2027 Update, click Next and follow the onscreen instructions.</p>
          <p>To quit, close this window and reboot your computer.</p>
        </section>
        <section data-installer-content="license" hidden>
          <h2>License</h2>
          <pre data-license-text style="height: 150px; margin: 12px 0; padding: 12px; overflow: auto; white-space: pre-wrap; background: #111; border: 1px solid #555;">Loading license...</pre>
          <label style="display: inline-flex; align-items: center; gap: 8px;">
            <input type="checkbox" data-license-accepted disabled style="width: 18px; min-width: 18px; height: 18px; margin: 0; padding: 0;">
            I agree to the Envy Public License
          </label>
        </section>
        <section data-installer-content="options" hidden>
          <h2>Install options</h2>
          <p>Choose how Envy should be configured on this device.</p>
          <label style="display: flex; align-items: center; gap: 8px; margin: 16px 0;">
            <input type="radio" name="install-type" value="upgrade" checked style="width: 18px; min-width: 18px; height: 18px; margin: 0; padding: 0;">
            Install Envy Client
          </label>
          <label style="display: flex; align-items: center; gap: 8px;">
            <input type="radio" name="install-type" value="clean" style="width: 18px; min-width: 18px; height: 18px; margin: 0; padding: 0;">
            Install Envy Server
          </label>
        </section>
        <section data-installer-content="install" hidden>
          <h2>Ready to install</h2>
          <p data-install-summary>Your selected options will be used when you start the installation.</p>
          <p data-install-status aria-live="polite"></p>
        </section>

        <button type="button" class="install-button2" data-installer-recovery>Recovery</button>
        <button type="button" class="install-button" data-installer-next>Next</button>
      </div>
    </div>
  `;

  const window = createWindow(windowTitle, customHTML, 700, 450);
  const pages = ['introduction', 'license', 'options', 'install'];
  let currentPage = 'introduction';
  const licenseText = window.querySelector('[data-license-text]');
  const licenseCheckbox = window.querySelector('[data-license-accepted]');

  fetch('./LICENSE')
    .then(response => {
      if (!response.ok) {
        throw new Error(`License request failed with status ${response.status}`);
      }
      return response.text();
    })
    .then(text => {
      licenseText.textContent = text;
      licenseCheckbox.disabled = false;
    })
    .catch(error => {
      console.error('[Installer] Unable to load LICENSE:', error);
      licenseText.textContent = 'Unable to load the license. Please check that the root LICENSE file is available.';
    });

  function renderPage(page) {
    currentPage = page;
    window.querySelectorAll('[data-installer-page]').forEach(button => {
      const isActive = button.dataset.installerPage === page;
      button.style.fontSize = isActive ? '20px' : '16px';
      button.style.fontWeight = isActive ? '900' : '400';
      button.setAttribute('aria-current', isActive ? 'step' : 'false');
    });
    window.querySelectorAll('[data-installer-content]').forEach(content => {
      content.hidden = content.dataset.installerContent !== page;
    });

    const nextButton = window.querySelector('[data-installer-next]');
    nextButton.textContent = page === 'install' ? 'Install Envy' : 'Next';
    nextButton.disabled = page === 'license' && !licenseCheckbox.checked;
  }

  window.querySelectorAll('[data-installer-page]').forEach(button => {
    button.addEventListener('click', () => renderPage(button.dataset.installerPage));
  });

  licenseCheckbox.addEventListener('change', () => {
    renderPage(currentPage);
  });

  function showInstallationScreen() {
    closeWindow(window.id);

    const installationScreen = document.createElement('div');
    installationScreen.style.cssText = [
      'position: fixed',
      'inset: 0',
      'z-index: 10000',
      'display: flex',
      'align-items: center',
      'justify-content: center',
      'background: #0f0f0f',
      'color: white',
      'text-align: center'
    ].join(';');

    const installationMessage = document.createElement('div');
    const heading = document.createElement('h2');
    const warning = document.createElement('p');
    let percentage = 0;

    heading.textContent = `Installing Envy (${percentage}%)`;
    warning.textContent = 'Please do not turn off your computer while installing';
    warning.style.margin = '16px 0 0';
    installationMessage.append(heading, warning);
    installationScreen.appendChild(installationMessage);
    document.body.appendChild(installationScreen);

    const progressInterval = setInterval(() => {
      percentage += 1;
      heading.textContent = `Installing Envy (${percentage}%)`;
      if (percentage === 100) {
        clearInterval(progressInterval);
      }
    }, 100);
  }

  window.querySelector('[data-installer-next]').addEventListener('click', () => {
    const pageIndex = pages.indexOf(currentPage);
    if (currentPage === 'install') {
      showInstallationScreen();
      return;
    }
    renderPage(pages[pageIndex + 1]);
  });

  window.querySelector('[data-installer-recovery]').addEventListener('click', () => {
    loadScript('./UI/recovery.js');
  });

  renderPage(currentPage);
})();
document.body.style.backgroundColor = "#0f0f0fff";

document.querySelectorAll('.window-minimize-btn').forEach(el => el.style.display = 'none');
document.querySelectorAll('.window-maximize-btn').forEach(el => el.style.display = 'none');