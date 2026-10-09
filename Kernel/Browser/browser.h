#ifndef ENVY_BROWSER_H
#define ENVY_BROWSER_H
#include "../FS/CDFS/cdfs.h"
#include "../Drivers/Input/ps2.h"
/* First browser increment: local HTML and script/DOM bootstrap, text output. */
int browser_start(cdfs *filesystem, void (*write)(const char *));
/* Feed translated PS/2 set-1 bytes; returns 1 when Ctrl+End toggles the panel. */
int browser_key(uint8_t scancode);
void browser_poll(void);
void browser_input(const input_event *event);
#endif
