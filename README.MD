<p>Developer? We reccomend reading up on the <a href="/Docs/README.md">Envy Hybrid API Documentation</a>.</p>
<p>Found a bug? Read the <a href="CONTRIBUTING.md">Contributing README</a>.</p>
<h2>THIS IS A PRERELEASE VERSION!! EXPECT BUGS!!</h2>
<br>
<img src="./Assets/brand/big.webp" height="100">
<BR><BR>
<h1>Envy 2027 Update (10.0.4523)</h1>

> [!NOTE]
> We have limited resources/time at the moment, if you want to contribute you are free to do so.

<img src="Assets/readme/demos/3006.png" height="400px" alt="A screenshot of Envy, running several demonstration apps">
<p>This repo contains everything you need to run:</p>
<li>Envy Aiya</li>
<li>Envy Kernel</li>
<li>Envy Go (Envy Mobile UI)</li>
<li>Envy Legacy Compatibilty (Cherry Tree, eWinBox)</li>
<li>Envy WebUI</li>
<br>
<h2>Directory structure</h2>

```
/os/
├── API                   # WebUI mini-apps that communicate with standard apps
├── Apps                   # WebUI applications (such as Filer)
│   └── Server             # Envy Server apps
├── Assets                 # Images, audio, etc
├── Cores                  # WebUI applications (such as Filer)
├── Docs                   # Documentation
├── Kernel                 # Envy Kernel
│   └── Drivers            # Envy Kernel drivers
│   └── System             # Envy Kernel source code
│       └── AMD64          # x86_64 specific code
│       └── armv8          # ARM64 specific code
│       └── i386           # x86_32 specific code
│       └── Neutral        # Architecture neutral code
├── Libs                   # Libraries needed for Envy WebUI
├── Packages               # Apps for Envy Legacy Compatibility
│   └── apps               # Legacy apps
│   └── system             # Legacy services
│   └── ui                 # Legacy desktop
├── Registry               # Envy Registry
│   └── local              # Local registry
│       └── apps           # Envy-made registry
├── Styles                 # CSS for Envy WebUI
├── Submodules             # 3rd party repositories
├── Tools                  # Scripts, etc to help set up and run Envy
│   └── Compilation        # Scripts to help compile Envy Kernel
│   └── ML                 # Scripts to help train and run Envy Aiya
├── UI                     # Envy WebUI GUI
```
<h1>Self-hosting Envy WebUI (localhost)</h1>
<p>Read the <a href="./Docs/Localhost.md">Localhost documentation</a> for more information.</p>