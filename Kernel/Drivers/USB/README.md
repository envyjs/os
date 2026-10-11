# USB support and device coverage

The kernel discovers PCI host controllers, starts schedules/rings, and enumerates directly attached root-port devices. Host code is separated into `uhci.c`, `ohci.c`, `ehci.c`, and `xhci.c`; PCI discovery and device/class binding live in `usb.c`. `device.c` parses configuration/interface/endpoint descriptors, `hid.c` handles supported input reports, and `msc.c` implements the storage transport.

Supported paths:

- UHCI full/low-speed control, bulk, and polled interrupt transfers.
- OHCI full/low-speed control and bulk transfers. Up to four OHCI controllers receive dedicated DMA/schedule state. Periodic interrupt transfers are not implemented.
- EHCI high-speed control, bulk, and polled periodic transfers. EHCI hands full/low-speed root-port devices to an available companion controller.
- xHCI control, bulk, and polled interrupt transfers for USB 1.x, 2.0, and 3.x speeds, when controller DMA addresses fit below 4 GiB.
- HID boot-protocol keyboards and mice, translated into the shared desktop input queue.
- SCSI transparent Bulk-Only Mass Storage, including READ(10), WRITE(10), SYNCHRONIZE CACHE, and FAT32 mounting.

This is not support for every USB product. USB standardizes transport and device classes, but many products use vendor-specific protocols. The kernel does not yet enumerate hubs, run OHCI periodic/interrupt transfers, parse arbitrary HID report descriptors, or support hotplug, UASP, isochronous audio/video, printers, network adapters, cameras, or vendor-specific interfaces. Those require additional class drivers and, for vendor interfaces, device-specific protocol implementations.

Transfers are polled because the kernel has no USB interrupt path yet. xHCI accepts up to four scratchpad buffers and 32-byte context formats; hosts requiring more scratchpads or 64-byte contexts are skipped. Currently enumerated devices must be directly attached, except full/low-speed root-port devices can be handed to an available EHCI companion controller.

Remaining general-purpose work: finish OHCI periodic schedules; implement hub descriptor/control requests and recursive child-port enumeration; add disconnect/reconnect lifecycle and address/slot reclamation; parse HID report descriptors for non-boot and composite input devices; then add classes such as UASP, audio, CDC networking, and printers. Covering every USB device would also require an open-ended set of vendor-specific drivers.
