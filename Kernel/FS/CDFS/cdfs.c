#include "cdfs.h"
static uint16_t le16(const uint8_t *p) { return p[0] | (uint16_t)p[1] << 8; }
static uint32_t le32(const uint8_t *p) {
    return p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static uint32_t be32(const uint8_t *p) {
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
}
static int both16(const uint8_t *p) { return le16(p) == ((uint16_t)p[2] << 8 | p[3]); }
static int both32(const uint8_t *p) { return le32(p) == be32(p + 4); }
static int header(const uint8_t *p) {
    return p[1]=='C' && p[2]=='D' && p[3]=='0' && p[4]=='0' && p[5]=='1' && p[6]==1;
}
static int extent(cdfs *fs, const cdfs_node *n) {
    uint32_t blocks = n->size / 2048 + (n->size % 2048 != 0);
    return n->lba < fs->blocks && blocks <= fs->blocks - n->lba;
}
static int record(cdfs *fs, const uint8_t *p, uint32_t available, cdfs_node *n) {
    if (available < 34 || p[0] < 34 || p[0] > available || !p[32] ||
        33u + p[32] + ((p[32] & 1u) == 0) > p[0] ||
        !both32(p + 2) || !both32(p + 10) || !both16(p + 28)) return CDFS_INVALID;
    if (p[1] || p[26] || p[27] || (p[25] & 0x80) || le16(p + 28) != 1)
        return CDFS_UNSUPPORTED;
    n->lba = le32(p + 2); n->size = le32(p + 10); n->flags = p[25];
    return extent(fs, n) ? CDFS_OK : CDFS_INVALID;
}
int cdfs_mount(cdfs *fs, cdfs_read_block read, void *context) {
    if (!fs || !read) return CDFS_INVALID;
    fs->read = read; fs->context = context; fs->blocks = 0;
    for (uint32_t lba = 16; lba < 80; ++lba) {
        if (read(context, lba, fs->scratch)) return CDFS_IO;
        const uint8_t *p = fs->scratch;
        if (!header(p)) return CDFS_INVALID;
        if (p[0] == 255) return CDFS_NOT_FOUND;
        if (p[0] != 1) continue;
        if (!both32(p + 80) || !both16(p + 128) || le16(p + 128) != 2048 ||
            !both16(p + 120) || le16(p + 120) != 1 ||
            !both16(p + 124) || le16(p + 124) != 1 || p[881] != 1) return CDFS_UNSUPPORTED;
        fs->blocks = le32(p + 80);
        if (fs->blocks <= lba) { fs->blocks = 0; return CDFS_INVALID; }
        int result = record(fs, p + 156, 34, &fs->root);
        if (result || !(fs->root.flags & 2) || p[188] != 1 || p[189] != 0) {
            fs->blocks = 0; return result ? result : CDFS_INVALID;
        }
        return CDFS_OK;
    }
    return CDFS_INVALID;
}
static char upper(char c) { return c >= 'a' && c <= 'z' ? (char)(c - 'a' + 'A') : c; }
static int name_matches(const uint8_t *p, const char *name, uint32_t length) {
    uint32_t n = p[32];
    if (n == 1 && p[33] <= 1) return 0;
    int explicit_version = 0;
    for (uint32_t i = 0; i < length; ++i) if (name[i] == ';') explicit_version = 1;
    if (!explicit_version) for (uint32_t i = 0; i < n; ++i) if (p[33 + i] == ';') { n = i; break; }
    if (n != length) return 0;
    for (uint32_t i = 0; i < n; ++i) if (upper((char)p[33 + i]) != upper(name[i])) return 0;
    return 1;
}
static int find(cdfs *fs, const cdfs_node *dir, const char *name, uint32_t length, cdfs_node *out) {
    if (!(dir->flags & 2)) return CDFS_NOT_FOUND;
    for (uint32_t offset = 0; offset < dir->size;) {
        uint32_t remain = dir->size - offset;
        uint32_t limit = remain < 2048 ? remain : 2048;
        if (fs->read(fs->context, dir->lba + offset / 2048, fs->scratch)) return CDFS_IO;
        for (uint32_t pos = 0; pos < limit;) {
            const uint8_t *p = fs->scratch + pos;
            if (!p[0]) break; /* Padding ends this block, not the directory. */
            cdfs_node candidate;
            int result = record(fs, p, limit - pos, &candidate);
            if (result) return result;
            if (name_matches(p, name, length)) { *out = candidate; return CDFS_OK; }
            pos += p[0];
        }
        if (remain <= 2048) break;
        offset += 2048;
    }
    return CDFS_NOT_FOUND;
}
int cdfs_lookup(cdfs *fs, const char *path, cdfs_node *node) {
    if (!fs || !fs->blocks || !path || !node) return CDFS_INVALID;
    cdfs_node current = fs->root;
    while (*path) {
        while (*path == '/') ++path;
        if (!*path) break;
        const char *start = path;
        uint32_t length = 0;
        while (*path && *path != '/') { ++path; if (++length > 255) return CDFS_INVALID; }
        if ((length == 1 && start[0] == '.') ||
            (length == 2 && start[0] == '.' && start[1] == '.')) return CDFS_UNSUPPORTED;
        int result = find(fs, &current, start, length, &current);
        if (result) return result;
        if (*path == '/' && !(current.flags & 2)) return CDFS_NOT_FOUND;
    }
    *node = current;
    return CDFS_OK;
}
int cdfs_read(cdfs *fs, const cdfs_node *node, uint32_t offset,
              void *buffer, uint32_t count, uint32_t *bytes_read) {
    if (bytes_read) *bytes_read = 0;
    if (!fs || !fs->blocks || !node || (!buffer && count) || !bytes_read || !extent(fs, node))
        return CDFS_INVALID;
    if (node->flags & 2) return CDFS_NOT_FILE;
    if (node->flags & 0x80) return CDFS_UNSUPPORTED;
    if (offset >= node->size) return CDFS_OK;
    if (count > node->size - offset) count = node->size - offset;
    uint8_t *dest = buffer;
    while (count) {
        uint32_t within = offset % 2048, chunk = 2048 - within;
        if (chunk > count) chunk = count;
        if (fs->read(fs->context, node->lba + offset / 2048, fs->scratch)) return CDFS_IO;
        for (uint32_t i = 0; i < chunk; ++i) dest[i] = fs->scratch[within + i];
        dest += chunk; offset += chunk; count -= chunk; *bytes_read += chunk;
    }
    return CDFS_OK;
}
int cdfs_list(cdfs *fs, const cdfs_node *dir, cdfs_visit visit, void *context) {
    if (!fs || !fs->blocks || !dir || !visit || !extent(fs, dir)) return CDFS_INVALID;
    if (!(dir->flags & 2)) return CDFS_NOT_FILE;
    for (uint32_t offset = 0; offset < dir->size;) {
        uint32_t remain = dir->size - offset;
        uint32_t limit = remain < CDFS_BLOCK_SIZE ? remain : CDFS_BLOCK_SIZE;
        if (fs->read(fs->context, dir->lba + offset / CDFS_BLOCK_SIZE, fs->scratch)) return CDFS_IO;
        for (uint32_t pos = 0; pos < limit;) {
            const uint8_t *p = fs->scratch + pos;
            if (!p[0]) break;
            cdfs_node child;
            int status = record(fs, p, limit - pos, &child);
            if (status) return status;
            if (!(p[32] == 1 && p[33] <= 1)) {
                char name[256]; uint32_t size = p[32];
                for (uint32_t i = 0; i < size; ++i) name[i] = (char)p[33 + i];
                for (uint32_t i = 0; i < size; ++i) if (name[i] == ';') { size = i; break; }
                name[size] = 0;
                if (!visit(context, name, &child)) return CDFS_OK;
            }
            pos += p[0];
        }
        if (remain <= CDFS_BLOCK_SIZE) break;
        offset += CDFS_BLOCK_SIZE;
    }
    return CDFS_OK;
}
