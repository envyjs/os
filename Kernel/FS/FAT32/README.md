FAT32 works with the polling AHCI SATA driver in `Drivers/Storage/sata.c`.
The boot kernel detects SATA disks and mounts the first FAT32 primary partition
on each disk, or a FAT32 volume starting at sector zero. Discovery only reads.
The browser still loads its resources from the boot CD's CDFS filesystem.

The SATA driver discovers PCI AHCI controllers, performs BIOS ownership handoff,
identifies disks, and uses READ/WRITE DMA EXT with 48-bit LBAs. It batches up to
64 sectors per DMA command and provides cache flushing. DMA buffers are aligned
static physical memory under the kernel's current identity mapping. Operations
must be serialized; completion is polled with bounded waits. Only 512-byte
logical sectors are supported. ATAPI optical drives continue to use the IDE
driver; AHCI optical drives, NCQ, hotplug, and interrupt-driven completion are
not implemented.

FAT32 validates the BPB and volume bounds, follows fragmented cluster chains,
lists directories, resolves nested paths and UTF-16 long filenames, and accepts
8.3 aliases. Name comparisons fold ASCII case. `fat32_create` creates regular
files with 8.3 names in existing directories. `fat32_write` overwrites or appends
data and allocates zeroed clusters as needed, updates FAT mirrors or the active
FAT, commits directory metadata, refreshes FSInfo hints, and flushes the disk.
It rejects sparse writes (offsets beyond EOF). Pass a null write callback for a
read-only mount. Formatting, GPT/extended partitions, directory creation,
deletion/truncation, long-name creation, and crash-atomic transactions are not
implemented. Successful writes persist; an I/O failure can leave partial data
or an allocated but unlinked cluster, as with an unjournaled filesystem.

```c
fat32 volume;
fat32_node file;
uint32_t bytes;
int status = fat32_mount(&volume, sata_read, sata_write, sata_flush,
                         &disk, disk.sectors);
if (!status) status = fat32_lookup(&volume, "/README.TXT", &file);
if (!status) status = fat32_read(&volume, &file, 0, buffer, sizeof(buffer), &bytes);
```

Run `make storage-test`. The fixture generator creates disposable 64 MiB FAT32
volumes under `Build/Tests/Storage`. QEMU snapshot overlays keep the base images
unchanged. Tests cover PC and Q35, MBR and sector-zero volumes, nested paths,
long names and aliases, fragmented and partial reads, FAT corruption rejection,
read-only mounts, file creation, append, directory growth, FAT mirroring, active
FAT selection, FSInfo updates, and transfers spanning multiple DMA commands.

References: [Intel AHCI 1.3.1](https://www.intel.com/content/dam/www/public/us/en/documents/technical-specifications/serial-ata-ahci-spec-rev1-3-1.pdf),
[Microsoft FAT on-disk definitions](https://github.com/microsoft/Windows-driver-samples/blob/main/filesys/fastfat/fat.h).
