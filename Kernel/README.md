# Native kernel bootstrap

`Runtime/JS/` implements a bounded JavaScript subset with named function
declarations, anonymous and named function expressions, parameters, calls,
return values, default parameters, arrow functions, and lexical closures.
Braced `if`/`else` branches support truthy conditions and strict `===`/`!==`
comparisons. String comparisons use their contents; booleans are distinct
from numbers. Object literals, constant bindings, and template literals
are also supported. Declarations are hoisted within each
script or function body; named expressions keep their name local to the call.
Calls isolate local bindings and retain captured environments for closures.
`js_call` invokes browser callbacks without arguments. Source buffers must
remain alive while functions defined in them are usable.

Function records and property storage are allocated on demand from a shared
1 MiB runtime arena; there is no fixed function or property count limit.
Strings, properties, and function records share
that memory, and exhaustion reports `JavaScript memory exhausted`.
The runtime supports at most eight parameters/arguments, 128
retained call environments, and sixteen nested execution frames. Environments
are retained until `js_init`; there is no garbage collection. Classes,
async/await, rest parameters, and general JavaScript control flow are unsupported.
`make test` includes function and closure regression tests in the QEMU guest.
In the native browser, Ctrl+End toggles a panel showing recent script errors
and `console.error` output. Both Ctrl keys work; closing the panel restores
the page. Keyboard input uses polled PS/2 set-1 scancodes.
Scripts share a 2 MiB source arena with no per-script size or count cap;
loading reports an error when available source memory is exhausted. The
browser exposes local CDFS `fetch` HEAD requests and synchronous `then`/`catch`
callbacks for the environment loader. Network fetch and full Promises are
not implemented.

All native kernel and bootloader sources live in `Kernel/`. Build utilities
live in `Tools/`; generated files live in `Build/`.

Page text uses the bundled Adwaita Sans, ReadexPro, and Ubuntu Mono TrueType
fonts. Local `@font-face` URLs resolve relative to their stylesheet;
`font-family` lists and inheritance select the font. Unicode character maps,
quadratic outlines, composite glyphs, glyph advances, and antialiased coverage
are supported. Font hinting, shaping, variable-font axes, and synthetic font
weights are not implemented. `make font-test` exercises all three font files.

The browser renders local JPEG, PNG, and SVG `<img>` elements and CSS backgrounds.
`background-image`, `background-size` (cover, contain, auto, lengths, percentages),
one/two-value `background-position`, `background-repeat` (repeat, repeat-x,
repeat-y, no-repeat), and single-layer `background` shorthand are supported.
Background URLs resolve relative to the stylesheet, and images blend over the
background color. Multiple background layers, CSS gradients, SVG text, filters,
and SVG scripts are not supported. The SVG subset includes paths, shapes,
transforms, fills, strokes, and gradients. JPEG output is downsampled to a maximum
2048-pixel dimension while retaining intrinsic dimensions for layout.
Decoding permits files up to 8 MiB and 32 million pixels within the shared image
memory budget. A 16 MiB LRU cache keeps decoded images across clock updates.

Repaints record drawing commands, sort them by `z-index` and document order,
and compose a frame before copying changed spans to the display. This keeps
negative-layer wallpaper behind earlier watermark overlays and avoids displaying
the intermediate clear. Timer callbacks that leave visible values unchanged do
not repaint. Stacking is a bounded subset rather than full CSS stacking contexts.
`make image-test` verifies decoders, alpha blending, clipping, background sizing,
tiling, watermark layering, and image reuse across clock updates in QEMU.

Native image builds generate 1920-pixel wallpaper previews under
`Build/Boot/x86/Previews/` using `Tools/prepare-images.ps1` and System.Drawing.
Only the packaged wallpaper copy is resized; repository assets stay original.
Decoder tests package original files, including the 5120-pixel wallpaper.
The native image requires 256 MiB RAM; `make run` and test runners configure it.

On Windows 11, install Clang (including LLD), NASM, and GNU Make and add their executable
directories to PATH. Run `make` from the project root to build `Kernel/`.
The root makefile also recognizes NASM at `C:/Program Files/NASM/nasm.exe`
when no custom NASM command is set.
The root makefile calls `Kernel/Makefile`, which builds the x86 bootloader.
Add future kernel components as dependencies in `Kernel/Makefile`.
You can override tool commands with make variables, for example
`make NASM='"C:/Program Files/NASM/nasm.exe"'` from PowerShell.
The bootloader uses NASM for real-mode setup and Clang for freestanding
x86-32 C code. LLD links the C payload. `Tools/mkimage.bat` invokes NASM on
`Tools/iso.asm` to create an ISO 9660 level-1 filesystem and El Torito boot
catalog. No PowerShell packer, external ISO utility, host C runtime, or
third-party bootloader is needed. `NASM`, `CLANG`, and `LLD` can be overridden
as make variables.

The result is `Build/Boot/x86/boot.iso`, a bootable CD containing the Envy tree.
The isolated CDFS test image is 256 KiB.
With QEMU installed and its directory on PATH, run this from the project root
to build the image and launch it:

```
make run
```

`QEMU` overrides the emulator executable (including an absolute path), and
`QEMU_FLAGS` adds options, for example `make run QEMU_FLAGS="-m 512M"`.
`make run` explicitly opens an SDL display window titled `Kernel x86-32`.
Use `make run QEMU_DISPLAY=gtk` to select GTK instead, or
`make run QEMU_DISPLAY=none` for a headless session.

This is a custom legacy BIOS CD loader. El Torito uses no-emulation mode:
BIOS loads the first 2048 bytes at `0x7c00`, and stage one uses extended BIOS
disk reads to load ninety-six 2048-byte CD blocks at `0x8000`, one block at
a time so BIOS reads never cross a 64 KiB boundary. It retries reads
and displays disk errors. Stage two selects VGA text mode,
enables A20 through port `0x92`, installs a flat GDT,
enters protected mode, and establishes a stack at `0x90000`. Its C payload
discovers IDE ATAPI optical drives, mounts the first readable CDFS CD, and
reads `/DOCS/README.TXT` directly from hardware to the screen, then halts with
interrupts disabled. There is no kernel executable loading or UEFI path yet.
The fast A20 gate assumes a PC-compatible machine that implements port `0x92`;
fallback A20 methods and verification remain future work.

Memory reserved for this bootstrap: stack below `0x7c00`, initial boot image at
`0x7c00..0x83ff` (overwritten after leaving stage one), stage two and its C
payload at `0x8000..0x37fff`, protected-mode
stack below `0x90000`, and VGA text memory at `0xb8000`. BIOS boot drive `DL` is saved by
stage two, but there is no kernel handoff ABI yet. No interrupts may be enabled
before installing an IDT and configuring interrupt controllers.

Architecture-specific sources are under `Boot/x86/`. A future x86-64 path can
reuse the BIOS entry stage, then add CPU checks, page tables, and a long-mode
transition. The current image contains the bootloader and demonstration file.

`FS/CDFS/` provides a read-only ISO 9660 driver: `cdfs_mount`, `cdfs_lookup`,
and `cdfs_read`. Each mount supplies a callback that reads one 2048-byte block;
the driver does not depend on BIOS, IDE, or a particular CD controller.
Directory lookup supports nested paths, case-insensitive ISO names, and an
optional `;1` version suffix. Reads accept byte offsets, cross block boundaries,
stop at EOF, and report partial reads on I/O failure. Numeric byte order pairs,
directory record lengths, and extents are validated. A mount owns one scratch
buffer, so callers must serialize operations on the same mount.

This first driver supports single-volume ISO 9660 with 2048-byte blocks and
contiguous files. It rejects extended attribute records, interleaved files,
multi-extent files, and multi-volume sets. Joliet, Rock Ridge, UDF, writing,
and `.`/`..` path resolution are not implemented. The descriptor scan is
bounded to 64 descriptors. Normal boot has no ISO RAM cache or 64-block
transport limit; the current image generator still creates a 64-block ISO.

`Drivers/Storage/atapi.c` provides native optical-device support using ATA
port I/O and ATAPI PIO packet commands. It enumerates PCI IDE controllers,
uses legacy ports for compatibility-mode channels, and reads assigned I/O
BARs for native-mode channels. Legacy ISA IDE ports are a fallback when no
PCI IDE controller exists. Both master and slave positions are probed, up to
eight channels and sixteen optical drives. ATA disks and non-optical ATAPI
devices are skipped. `IDENTIFY PACKET DEVICE` identifies the drive and packet
length; `READ CAPACITY(10)` obtains the media size; `READ(12)` reads one
2048-byte block for CDFS. Empty drives are skipped and another drive is tried.
The first readable ISO is selected; BIOS drive numbers are not mapped to
native IDE devices yet, so multiple inserted bootable CDs should be avoided.

PIO operations use bounded polling with device interrupts disabled. Packet
phase direction and length are validated, REQUEST SENSE handles unit
attention and absent media, and stuck/protocol-error transactions reset the
channel. Poll budgets are iteration limits rather than calibrated wall-clock
timeouts. Media changes invalidate the device's capacity and require calling
`atapi_media_ready` and remounting CDFS. Serialize all commands sharing an IDE
channel. Bootstrap discovery resets both devices on each discovered channel.
DMA, IRQ-driven transfers, AHCI-only SATA controllers, USB optical drives,
PCI MMIO IDE controllers, and physical hardware validation are future work.
SATA drives exposed through IDE compatibility mode use this ATAPI path.

Run `make test` for headless QEMU tests of the driver. It builds a separate
ISO under `Build/Tests/CDFS/`, reads it through native ATAPI into a test-only
RAM fixture, mutates only that fixture, and returns success
only when all guest tests pass. The normal GUI image is unaffected. Tests
cover directory padding, nested lookup, versions, EOF, partial I/O, multi-block
reads, corrupt records, byte order mismatches, and out-of-volume extents.
Native tests run with the CD at each of the four IDE positions, then with an
ATA disk and empty optical drive alongside the boot CD. They verify drive
identification, capacity, first/last accessible data, invalid-LBA rejection,
and mounting and reading CDFS through native I/O. Debug output is saved to
`Build/Tests/CDFS/atapi-*.log`. Native PCI channel discovery is implemented;
the QEMU test matrix exercises PIIX IDE compatibility-mode channels.

Format references: [ECMA-119](https://ecma-international.org/publications-and-standards/standards/ecma-119/)
and [El Torito](https://www.scs.stanford.edu/05au-cs240c/lab/boot-cdrom.pdf).
ATA/ATAPI command and identify layouts were checked against
[EDK II definitions](https://github.com/tianocore/edk2/blob/master/MdePkg/Include/IndustryStandard/Atapi.h).
