<div style="display: flex;">
<img height="20" src="https://img.shields.io/github/languages/count/envyjs/os"/>
<img height="20" src="https://img.shields.io/github/repo-size/envyjs/os"/>
</div>
<span style="display:block;text-align:center"><img src="./Assets/readme/banner.png"></span>
<p>Developer? We reccomend reading up on the <a href="/Docs/README.md">Envy Hybrid API Documentation</a>.</p>

> [!NOTE]
> We have limited resources/time at the moment, if you want to contribute you are free to do so.

> [!NOTE]
> Please note that this is also a pre-release version of Envy and that bugs and issues may arise.

<p>Envy is a upcoming OS for any type of device, wherever it is a measly thin PC or a full blown server or workstation, Envy likely has a version for that.</p>
<img src="Assets/readme/demos/4523.png" height="400px" alt="A screenshot of the Envy desktop flavor, running several demonstration apps">
<p>This repo contains everything you need to run:</p>
<li>Envy Aiya</li>
<li>Envy Kernel</li>
<li>Envy Go (Envy Mobile UI)</li>
<li>Envy Legacy Compatibilty (Cherry Tree, eWinBox)</li>
<li>Envy WebUI</li>
<h2>Download Envy</h2>
<table class="tg"><thead>
  <tr>
    <th class="tg-0pky">CPU</th>
    <th class="tg-0pky">Machines</th>
    <th class="tg-0pky">How to get</th>
  </tr></thead>
<tbody>
  <tr>
    <td class="tg-0pky">amd64</td>
    <td class="tg-0pky">64-bit x86-family machines with AMD or Intel CPUs</td>
    <td class="tg-0pky">ISO image, compile guide</td>
  </tr>
  <tr>
    <td class="tg-0pky">arm</td>
    <td class="tg-0pky">Nokia/Microsoft Lumia line of phones, original Raspberry Pi</td>
    <td class="tg-0pky">ISO image, WIM image, compile guide</td>
  </tr>
  <tr>
    <td class="tg-0pky">arm64</td>
    <td class="tg-0pky">Microsoft Lumia 950 only for now</td>
    <td class="tg-0pky">ISO image, WIM image, compile guide</td>
  </tr>
  <tr>
    <td class="tg-0pky">i386</td>
    <td class="tg-0pky">32-bit x86-family generic machines ("PC clones")</td>
    <td class="tg-0pky">ISO image, compile guide</td>
  </tr>
  <tr>
    <td class="tg-0pky">mips</td>
    <td class="tg-0pky">PlayStation 2 game console</td>
    <td class="tg-0pky">ISO image, ELF file, compile guide</td>
  </tr>
</tbody></table>
<h2>Directory structure</h2>

```
/os/
├── API                    # WebUI mini-apps that communicate with standard apps
├── Apps                   # WebUI applications (such as Filer)
│   └── Server             # Envy Server apps
│   └── Go                 # Envy Go apps
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
├── Platforms              # Ports of Envy WebUI to other platforms
│   └── ps2                # PlayStation 2 port
├── Registry               # Envy Registry
│   └── local              # Local registry
│       └── Envy           # Envy-made registry
├── Styles                 # CSS for Envy WebUI
├── Submodules             # 3rd party repositories
├── Tools                  # Scripts, etc to help set up and run Envy
│   └── Compilation        # Scripts to help compile Envy Kernel
│   └── ML                 # Scripts to help train and run Envy Aiya
├── UI                     # Envy WebUI GUI
```
<h1>Self-hosting Envy WebUI (localhost)</h1>
<p>Read the <a href="./Docs/Localhost.md">Localhost documentation</a> for more information.</p>