(function() {
  const windowTitle = 'Terminal';
  const customHTML = `
  <div id="terminal"></div>
  `;

  // Create a window dynamically
  createWindow(windowTitle, customHTML, [
    { label: 'Close', action: closeWindow },
    { label: 'Minimize', action: minimizeWindow },
    { label: 'Maximize', action: maximizeWindow }
  ]);
})();

const terminal = document.getElementById('terminal');

    const commands = {
      help: () => {
        createOutputLine("Built-in commands:");
        createOutputLine("(clear, cls), date, echo, ver");
      },
      echo: (args) => args.join(' '),
      date: () => new Date().toLocaleString(),
      ver: () => {createOutputLine("Envy 10 2025 Update 10.0.1768");},
      clear: () => { terminal.innerHTML = ''; },
      cls: () => { terminal.innerHTML = ''; },
    };

    const createOutputLine = (text, cssClass = 'output') => {
      const line = document.createElement('div');
      line.textContent = text;
      line.className = cssClass;
      terminal.appendChild(line);
    };

    const createInputLine = () => {
      const inputWrapper = document.createElement('div');
      inputWrapper.className = 'input';

      const prompt = document.createElement('span');
      prompt.textContent = '>';
      prompt.className = 'prompt';

      const input = document.createElement('input');
      input.type = 'text';

      inputWrapper.appendChild(prompt);
      inputWrapper.appendChild(input);
      terminal.appendChild(inputWrapper);
      input.focus();

      input.addEventListener('keydown', (e) => {
        if (e.key === 'Enter') {
          const commandLine = input.value.trim();
          processCommand(commandLine);
          inputWrapper.remove();
          createInputLine();
        }
      });
    };

    const processCommand = (commandLine) => {
      if (!commandLine) return;

      const [command, ...args] = commandLine.split(' ');
      createOutputLine(`> ${commandLine}`, 'prompt');

      if (commands[command]) {
        const result = typeof commands[command] === 'function'
          ? commands[command](args)
          : commands[command];
        if (result !== undefined) createOutputLine(result);
      } else {
        createOutputLine(`"${command}" is not a known command or file`);
      }
    };

    // Initialize the terminal
    createOutputLine("Envy [Version 10.0.1768]");
    createOutputLine("Copyright (C) Envy Group 2022-2025")
    createInputLine();