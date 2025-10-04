(function() {
  const windowTitle = 'Terminal';
  const customHTML = `
  <div id="terminal"></div>
  `;

  // Create a window dynamically
  createWindow(windowTitle, customHTML, 500, 300);
})();

const terminal = document.getElementById('terminal');

    const commands = {
      help: () => {
        createOutputLine("Envy Terminal, version v10.0.4304 (2027 Update)");
        createOutputLine("These shell commands are defined internally.  Type `help' to see this list.");
        createOutputLine("Type `help name' to find out more about the function `name'.");
        createOutputLine("(clear, cls), date, echo, (exec, start), help, recenv, serve, uname, uptime, ver");
      },
      echo: (args) => args.join(' '),
      date: () => new Date().toLocaleString(),
      exec: (args) => {
        if (args.length === 0) {
          return "Usage: exec <app_path>";
        }
        const scriptPath = args[0];
        loadScript(scriptPath);
      },
      start: (args) => {
        if (args.length === 0) {
          return "Usage: start <app_path>";
        }
        const scriptPath = args[0];
        loadScript(scriptPath);
      },
      uname: (args) => {
        if (args[0] === '-a') {
          return "Envy 10.0.4304 2027 Update Neutral Envy";
        } if (args[0] === '-r') {
          return "10.0.4304";
        } else {
          return "Envy";
        } },
      recenv: () => {createOutputLine("Starting Recovery Environment..."); loadScript('./UI/recovery.js');},
      serve: () => {createOutputLine("Starting Envy Server..."); loadScript('./UI/server.js');},
      uptime: () => {
        const now = new Date();
        const uptimeSeconds = Math.floor((now - performance.timing.navigationStart) / 1000);
        const hours = Math.floor(uptimeSeconds / 3600);
        const minutes = Math.floor((uptimeSeconds % 3600) / 60);
        const seconds = uptimeSeconds % 60;
        const milliseconds = Math.floor((now - performance.timing.navigationStart) % 1000);
        return `Uptime: ${hours}h ${minutes}m ${seconds}s ${milliseconds}ms`;
      },
      ver: () => {createOutputLine("Envy 10 2027 Update 10.0.4304");},
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
      input.className = 'ti';

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
    createOutputLine("Envy [Version 10.0.4304]");
    createOutputLine("Copyright (C) Envy Group 2022-2027")
    createInputLine();