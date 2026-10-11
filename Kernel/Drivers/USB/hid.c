#include "hid.h"
#include "../../Drivers/Input/ps2.h"

typedef struct { uint8_t code, extended; } scan_code;
static uint8_t previous_keys[6], previous_modifiers, locks;

static scan_code translate(uint8_t usage) {
    static const uint8_t letters[26] = {
        0x1c,0x32,0x21,0x23,0x24,0x2b,0x34,0x33,0x43,0x3b,0x42,0x4b,0x3a,
        0x31,0x44,0x4d,0x15,0x2d,0x1b,0x2c,0x3c,0x2a,0x1d,0x22,0x35,0x1a
    };
    if (usage >= 0x04 && usage <= 0x1d) return (scan_code){letters[usage-4],0};
    if (usage >= 0x1e && usage <= 0x26) return (scan_code){(uint8_t)(usage-0x1c),0};
    switch (usage) {
    case 0x27: return (scan_code){0x45,0}; /* 0 */
    case 0x28: return (scan_code){0x5a,0};
    case 0x29: return (scan_code){0x76,0};
    case 0x2a: return (scan_code){0x66,0};
    case 0x2b: return (scan_code){0x0d,0};
    case 0x2c: return (scan_code){0x29,0};
    case 0x2d: return (scan_code){0x4e,0};
    case 0x2e: return (scan_code){0x55,0};
    case 0x2f: return (scan_code){0x54,0};
    case 0x30: return (scan_code){0x5b,0};
    case 0x31: return (scan_code){0x5d,0};
    case 0x33: return (scan_code){0x4c,0};
    case 0x34: return (scan_code){0x52,0};
    case 0x35: return (scan_code){0x0e,0};
    case 0x36: return (scan_code){0x41,0};
    case 0x37: return (scan_code){0x49,0};
    case 0x38: return (scan_code){0x4a,0};
    case 0x39: return (scan_code){0x58,0};
    case 0x3a: return (scan_code){0x05,0};
    case 0x3b: return (scan_code){0x06,0};
    case 0x3c: return (scan_code){0x04,0};
    case 0x3d: return (scan_code){0x0c,0};
    case 0x3e: return (scan_code){0x03,0};
    case 0x3f: return (scan_code){0x0b,0};
    case 0x40: return (scan_code){0x83,0};
    case 0x41: return (scan_code){0x0a,0};
    case 0x42: return (scan_code){0x01,0};
    case 0x43: return (scan_code){0x09,0};
    case 0x44: return (scan_code){0x78,0};
    case 0x45: return (scan_code){0x07,0};
    case 0x46: return (scan_code){0x7c,0};
    case 0x47: return (scan_code){0x7e,0};
    case 0x48: return (scan_code){0x77,0};
    case 0x49: return (scan_code){0x70,1};
    case 0x4a: return (scan_code){0x6c,1};
    case 0x4b: return (scan_code){0x7d,1};
    case 0x4c: return (scan_code){0x71,1};
    case 0x4d: return (scan_code){0x69,1};
    case 0x4e: return (scan_code){0x7a,1};
    case 0x4f: return (scan_code){0x74,1};
    case 0x50: return (scan_code){0x6b,1};
    case 0x51: return (scan_code){0x72,1};
    case 0x52: return (scan_code){0x75,1};
    case 0x53: return (scan_code){0x77,0};
    case 0x54: return (scan_code){0x4a,1};
    case 0x55: return (scan_code){0x7c,0};
    case 0x56: return (scan_code){0x7b,0};
    case 0x57: return (scan_code){0x79,0};
    case 0x58: return (scan_code){0x5a,1};
    default: return (scan_code){0,0};
    }
}

static uint8_t character(uint8_t usage, uint8_t modifiers) {
    static const char punctuation[] = "-=[]\\;'`,./";
    static const char shifted_punctuation[] = "_+{}|~:\"~<>?";
    if (usage >= 0x04 && usage <= 0x1d) {
        uint8_t c = (uint8_t)('a' + usage - 0x04);
        if (!!(modifiers & INPUT_SHIFT) != !!(modifiers & INPUT_CAPS)) c -= 32;
        return c;
    }
    if (usage >= 0x1e && usage <= 0x27) {
        uint32_t index = usage == 0x27 ? 9 : usage - 0x1e;
        return (uint8_t)((modifiers & INPUT_SHIFT) ? "!@#$%^&*()"[index] : "1234567890"[index]);
    }
    if (usage >= 0x2d && usage <= 0x38) {
        uint32_t index = usage - 0x2d;
        return (uint8_t)((modifiers & INPUT_SHIFT) ? shifted_punctuation[index] : punctuation[index]);
    }
    if (usage == 0x28) return '\n';
    if (usage == 0x2a) return '\b';
    if (usage == 0x2b) return '\t';
    if (usage == 0x2c) return ' ';
    return 0;
}

static uint8_t modifiers_from_report(uint8_t report) {
    uint8_t value = locks;
    if (report & 0x22) value |= INPUT_SHIFT;
    if (report & 0x11) value |= INPUT_CONTROL;
    if (report & 0x44) value |= INPUT_ALT;
    if (report & 0x88) value |= INPUT_SUPER;
    return value;
}

static void submit_key(uint8_t usage, int pressed, uint8_t modifiers) {
    scan_code scan = translate(usage);
    if (!scan.code) return;
    if (usage == 0x39 && pressed) locks ^= INPUT_CAPS;
    uint16_t key = (uint16_t)(scan.code + (scan.extended ? 0x100 : 0));
    input_event event = {
        .type = INPUT_KEY, .key = key,
        .scancode = (uint8_t)(scan.code | (pressed ? 0 : 0x80)),
        .character = pressed ? character(usage, modifiers) : 0,
        .modifiers = (uint8_t)(modifiers | locks), .pressed = (uint8_t)pressed
    };
    input_submit(&event);
}

void usb_hid_reset(void) {
    for (uint32_t i=0;i<6;i++) previous_keys[i]=0;
    previous_modifiers=0;locks=0;
}

int usb_hid_keyboard_report(const uint8_t *report, uint32_t length) {
    if (!report || length < 8) return 0;
    for (uint32_t i=2;i<8;i++) if (report[i] == 1) return 0; /* rollover */
    uint8_t modifiers = modifiers_from_report(report[0]);
    for (uint32_t i=0;i<6;i++) {
        uint8_t old=previous_keys[i];
        if (old) {
            int found=0;for(uint32_t j=2;j<8;j++)if(report[j]==old){found=1;break;}
            if(!found)submit_key(old,0,modifiers);
        }
    }
    for (uint32_t i=2;i<8;i++) {
        uint8_t usage=report[i];if(!usage)continue;int found=0;
        for(uint32_t j=0;j<6;j++)if(previous_keys[j]==usage){found=1;break;}
        if(!found)submit_key(usage,1,modifiers);
    }
    for(uint32_t i=0;i<6;i++)previous_keys[i]=report[i+2];
    previous_modifiers=report[0];(void)previous_modifiers;
    return 1;
}

int usb_hid_mouse_report(const uint8_t *report, uint32_t length) {
    if(!report||length<3)return 0;
    int32_t dx=(int8_t)report[1],dy=-(int32_t)(int8_t)report[2];
    int32_t wheel=length>=4?(int8_t)report[3]:0;
    input_pointer_delta(dx,dy,report[0]&7u,wheel);return 1;
}
