#include "atapi.h"
#include "../../Arch/x86/io.h"

#define BUSY 0x80u
#define DRQ 0x08u
#define FAULT 0x20u
#define ERROR 0x01u
#define POLL_LIMIT 2000000u
#define MAX_CHANNELS 8u
#define READ_AHEAD_BLOCKS 16u
static struct {atapi_device *device;uint32_t lba,count,stamp;uint8_t data[READ_AHEAD_BLOCKS*ATAPI_BLOCK_SIZE];} read_cache[4];
static uint32_t cache_clock,read_commands,cache_hits;
uint32_t atapi_read_command_count(void){return read_commands;}
uint32_t atapi_cache_hit_count(void){return cache_hits;}
static void invalidate_reads(atapi_device *device){for(uint32_t i=0;i<4;i++)if(!device||read_cache[i].device==device)read_cache[i].count=0;}
typedef struct { uint16_t io, control; } channel;

static void settle(uint16_t control) {
    for (uint32_t i = 0; i < 4; ++i) (void)in8(control);
}
/* Poll alternate status; regular status is read only when acknowledging a phase. */
static int wait_idle(const atapi_device *device, uint8_t *status) {
    for (uint32_t i = 0; i < POLL_LIMIT; ++i) {
        *status = in8(device->control);
        if (*status == 0xff) return ATAPI_DEVICE_ERROR;
        if (!(*status & BUSY)) return ATAPI_OK;
    }
    return ATAPI_TIMEOUT;
}
static int wait_drq(const atapi_device *device) {
    for (uint32_t i = 0; i < POLL_LIMIT; ++i) {
        uint8_t status = in8(device->control);
        if (status == 0xff) return ATAPI_DEVICE_ERROR;
        if (status & BUSY) continue;
        if (status & (ERROR | FAULT)) return ATAPI_DEVICE_ERROR;
        if (status & DRQ) return ATAPI_OK;
    }
    return ATAPI_TIMEOUT;
}
static int select_device(const atapi_device *device) {
    uint8_t status;
    int result = wait_idle(device, &status);
    /* An absent selected master may float the bus; its slave can still exist. */
    if (result && result != ATAPI_DEVICE_ERROR) return result;
    out8(device->io + 6, (uint8_t)(0xa0 | device->drive << 4));
    settle(device->control);
    return wait_idle(device, &status);
}
static int reset_channel(channel ch) {
    out8(ch.control, 6); /* SRST and nIEN: no IRQ handlers installed yet. */
    for (uint32_t i = 0; i < 16; ++i) io_delay();
    out8(ch.control, 2);
    for (uint32_t i = 0; i < 2000; ++i) io_delay();
    atapi_device device = { .io = ch.io, .control = ch.control };
    uint8_t status;
    return wait_idle(&device, &status);
}
static uint32_t pci_read(uint32_t address, uint8_t offset) {
    out32(0xcf8, 0x80000000u | address | (offset & 0xfcu));
    return in32(0xcfc);
}
static void add_channel(channel *channels, uint32_t *count, uint16_t io, uint16_t control) {
    if (!io || !control || *count == MAX_CHANNELS) return;
    for (uint32_t i = 0; i < *count; ++i) if (channels[i].io == io) return;
    channels[(*count)++] = (channel){ io, control };
}
static void pci_channel(channel *channels, uint32_t *count, uint32_t address,
                        uint8_t index, uint8_t native) {
    if (!native) {
        add_channel(channels, count, index ? 0x170 : 0x1f0, index ? 0x376 : 0x3f6);
        return;
    }
    uint32_t io = pci_read(address, (uint8_t)(0x10 + index*8));
    uint32_t control = pci_read(address, (uint8_t)(0x14 + index*8));
    if (!(io & 1) || !(control & 1) || !(io & ~3u) || !(control & ~3u) || (io & ~3u) > 0xfff8 ||
        (control & ~3u) > 0xfffc) return; /* Only assigned I/O BARs are supported. */
    add_channel(channels, count, (uint16_t)(io & ~3u), (uint16_t)((control & ~3u) + 2));
}
static uint32_t discover_channels(channel *channels) {
    uint32_t count = 0;
    int found_ide = 0;
    for (uint32_t bus = 0; bus < 256; ++bus) for (uint32_t slot = 0; slot < 32; ++slot) {
        uint32_t address = bus << 16 | slot << 11;
        if ((pci_read(address, 0) & 0xffff) == 0xffff) continue;
        uint32_t functions = (pci_read(address, 0x0c) & 0x00800000) ? 8 : 1;
        for (uint32_t function = 0; function < functions; ++function) {
            uint32_t target = address | function << 8;
            if ((pci_read(target, 0) & 0xffff) == 0xffff) continue;
            uint32_t type = pci_read(target, 8);
            if ((type >> 16) != 0x0101) continue;
            found_ide = 1;
            uint16_t command = (uint16_t)pci_read(target, 4);
            out32(0xcf8, 0x80000000u | target | 4);
            out16(0xcfc, (uint16_t)(command | 1)); /* Enable I/O, preserve other command bits. */
            uint8_t mode = (uint8_t)(type >> 8);
            pci_channel(channels, &count, target, 0, mode & 1);
            pci_channel(channels, &count, target, 1, mode & 4);
        }
    }
    if (!found_ide) {
        add_channel(channels, &count, 0x1f0, 0x3f6);
        add_channel(channels, &count, 0x170, 0x376);
    }
    return count;
}
static int identify(atapi_device *device) {
    uint8_t status;
    int result = select_device(device);
    if (result) return result;
    status = in8(device->io + 7);
    if (status == 0xff) return ATAPI_DEVICE_ERROR;
    /* ATAPI may report status zero after reset; use its task-file signature. */
    uint8_t mid = in8(device->io + 4), high = in8(device->io + 5);
    if (!((mid == 0x14 && high == 0xeb) || (mid == 0x69 && high == 0x96)))
        return ATAPI_UNSUPPORTED;
    out8(device->io + 1, 0);
    /* IDENTIFY needs no task-file arguments. Writes would also erase the
       unselected sibling's post-reset signature on a shared ATA channel. */
    out8(device->io + 7, 0xa1); /* IDENTIFY PACKET DEVICE; ATA disks abort this. */
    settle(device->control);
    result = wait_drq(device);
    if (result) return result;
    uint16_t words[256];
    for (uint32_t i = 0; i < 256; ++i) words[i] = in16(device->io);
    settle(device->control);
    result = wait_idle(device, &status);
    if (result) return result;
    if (status & (ERROR | FAULT | DRQ)) return ATAPI_PROTOCOL;
    if (!(words[0] & 0x8000) || ((words[0] >> 8) & 0x1f) != 5 || (words[0] & 3) > 1)
        return ATAPI_UNSUPPORTED;
    device->packet_bytes = (words[0] & 3) ? 16 : 12;
    for (uint32_t i = 0; i < 20; ++i) {
        device->model[i*2] = (char)(words[27+i] >> 8);
        device->model[i*2+1] = (char)words[27+i];
    }
    device->model[40] = 0;
    for (uint32_t i = 40; i && device->model[i-1] == ' '; --i) device->model[i-1] = 0;
    return ATAPI_OK;
}
uint32_t atapi_probe(atapi_device *devices, uint32_t capacity) {
    invalidate_reads(0);
    if (!devices || !capacity) return 0;
    channel channels[MAX_CHANNELS];
    uint32_t count = discover_channels(channels), found = 0;
    for (uint32_t i = 0; i < count && found < capacity; ++i) {
        int reset = reset_channel(channels[i]);
        if (reset && reset != ATAPI_DEVICE_ERROR) continue;
        for (uint8_t drive = 0; drive < 2 && found < capacity; ++drive) {
            atapi_device candidate = { .io = channels[i].io, .control = channels[i].control, .drive = drive };
            int result = identify(&candidate);
            if (!result) devices[found++] = candidate;
        }
    }
    return found;
}
/* Read-only PIO packet transaction, validating every command/data/completion phase. */
static int packet_raw(atapi_device *device, const uint8_t *cdb, uint8_t *buffer, uint32_t size) {
    int result = select_device(device);
    if (result) return result;
    uint8_t status = in8(device->io + 7);
    if (status & DRQ) return ATAPI_PROTOCOL;
    out8(device->control, 2);
    out8(device->io + 1, 0); /* PIO, non-overlapped command. */
    out8(device->io + 2, 0);
    out8(device->io + 3, 0);
    uint16_t limit = (uint16_t)(size ? size : 2048);
    out8(device->io + 4, (uint8_t)limit);
    out8(device->io + 5, (uint8_t)(limit >> 8));
    out8(device->io + 7, 0xa0);
    settle(device->control);
    result = wait_drq(device);
    if (result) return result;
    if ((in8(device->io + 2) & 3) != 1) return ATAPI_PROTOCOL;
    for (uint32_t i = 0; i < device->packet_bytes; i += 2)
        out16(device->io, (uint16_t)(cdb[i] | (uint16_t)cdb[i+1] << 8));
    settle(device->control);
    uint32_t received = 0;
    for (uint32_t phase = 0; phase < 64; ++phase) {
        result = wait_idle(device, &status);
        if (result) return result;
        status = in8(device->io + 7);
        if (status & (ERROR | FAULT)) return ATAPI_DEVICE_ERROR;
        uint8_t reason = in8(device->io + 2) & 3;
        if (!(status & DRQ)) return reason == 3 && received == size ? ATAPI_OK : ATAPI_PROTOCOL;
        if (reason != 2) return ATAPI_PROTOCOL;
        uint32_t bytes = in8(device->io + 4) | (uint32_t)in8(device->io + 5) << 8;
        if (!bytes || bytes > size - received) return ATAPI_PROTOCOL;
        uint8_t *destination=buffer+received;uint32_t words=bytes/2;
        __asm__ volatile("cld; rep insw" : "+D"(destination), "+c"(words) : "d"(device->io) : "memory");
        if(bytes&1)buffer[received+bytes-1]=(uint8_t)in16(device->io);
        received += bytes;
        settle(device->control);
    }
    return ATAPI_TIMEOUT;
}
static int packet(atapi_device *device, const uint8_t *cdb, uint8_t *buffer, uint32_t size) {
    int result = packet_raw(device, cdb, buffer, size);
    if (result == ATAPI_TIMEOUT || result == ATAPI_PROTOCOL) {
        /* Abandon a stuck data phase before allowing another command. */
        (void)reset_channel((channel){ device->io, device->control });
        device->blocks = 0;
    }
    return result;
}
static int sense(atapi_device *device) {
    uint8_t cdb[16] = { 3, 0, 0, 0, 18 }, data[18];
    int result = packet(device, cdb, data, sizeof(data));
    if (result) return result;
    if ((data[0] & 0x7f) != 0x70 && (data[0] & 0x7f) != 0x71) return ATAPI_PROTOCOL;
    device->sense_key = data[2] & 0xf; device->asc = data[12]; device->ascq = data[13];
    return ATAPI_OK;
}
static uint32_t be32(const uint8_t *p) {
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
}
int atapi_media_ready(atapi_device *device) {
    invalidate_reads(device);
    if (!device || (device->packet_bytes != 12 && device->packet_bytes != 16)) return ATAPI_UNSUPPORTED;
    device->blocks = 0;
    device->sense_key = device->asc = device->ascq = 0;
    uint8_t cdb[16] = { 0x25 }, data[8]; /* READ CAPACITY(10). */
    for (uint32_t retry = 0; retry < 8; ++retry) {
        int result = packet(device, cdb, data, sizeof(data));
        if (!result) {
            uint32_t last = be32(data);
            if (be32(data + 4) != ATAPI_BLOCK_SIZE || last == 0xffffffffu) return ATAPI_UNSUPPORTED;
            device->blocks = last + 1;
            return ATAPI_OK;
        }
        if (result != ATAPI_DEVICE_ERROR) return result;
        result = sense(device);
        if (result) return result;
        if (device->sense_key == 2 && device->asc == 0x3a) return ATAPI_NO_MEDIA;
        if (device->sense_key != 6 && !(device->sense_key == 2 && device->asc == 4)) return ATAPI_DEVICE_ERROR;
        if (device->sense_key == 2) {
            uint8_t start[16] = { 0x1b, 0, 0, 0, 1 }; /* Request spin-up. */
            (void)packet(device, start, 0, 0);
            for (uint32_t i = 0; i < 100000; ++i) io_delay();
        }
    }
    return ATAPI_TIMEOUT;
}
int atapi_read_block(void *context, uint32_t lba, uint8_t *buffer) {
    atapi_device *device = context;
    if (!device || !buffer || !device->blocks || lba >= device->blocks) return ATAPI_RANGE;
    for(uint32_t i=0;i<4;i++)if(read_cache[i].device==device&&read_cache[i].count&&lba>=read_cache[i].lba&&lba-read_cache[i].lba<read_cache[i].count){
        uint32_t offset=(lba-read_cache[i].lba)*ATAPI_BLOCK_SIZE;for(uint32_t b=0;b<ATAPI_BLOCK_SIZE;b++)buffer[b]=read_cache[i].data[offset+b];read_cache[i].stamp=++cache_clock;cache_hits++;return ATAPI_OK;}
    uint32_t slot=0;for(uint32_t i=0;i<4;i++){if(!read_cache[i].count){slot=i;break;}if(read_cache[i].stamp<read_cache[slot].stamp)slot=i;}
    read_cache[slot].count=0;
    uint32_t first=lba&~(READ_AHEAD_BLOCKS-1u),count=device->blocks-first;if(count>READ_AHEAD_BLOCKS)count=READ_AHEAD_BLOCKS;
    uint8_t cdb[16] = { 0xa8 }; /* READ(12), bounded sequential read-ahead. */
    for (uint32_t i = 0; i < 4; ++i) cdb[2+i] = (uint8_t)(first >> (24 - 8*i));
    cdb[9] = (uint8_t)count;read_commands++;
    int result = packet(device, cdb, read_cache[slot].data, count*ATAPI_BLOCK_SIZE);
    if (result == ATAPI_DEVICE_ERROR) {
        int sense_result = sense(device);
        if (sense_result) return sense_result;
        if (device->sense_key == 2 && device->asc == 0x3a) { device->blocks = 0; return ATAPI_NO_MEDIA; }
        if (device->sense_key == 6) { device->blocks = 0; return ATAPI_DEVICE_ERROR; } /* Require remount on media change. */
    }
    if(!result){read_cache[slot].device=device;read_cache[slot].lba=first;read_cache[slot].count=count;read_cache[slot].stamp=++cache_clock;
        uint32_t offset=(lba-first)*ATAPI_BLOCK_SIZE;for(uint32_t b=0;b<ATAPI_BLOCK_SIZE;b++)buffer[b]=read_cache[slot].data[offset+b];}
    else invalidate_reads(device);
    return result;
}
