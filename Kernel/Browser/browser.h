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
#ifdef BOOT_IMAGE_TESTS
int browser_test_verify(const char *source);
void browser_test_advance(uint32_t milliseconds);
int browser_test_bounds(const char *selector,int32_t *x,int32_t *y,int32_t *w,int32_t *h);
int browser_test_scripts(cdfs *filesystem,const char *html,const char *const *paths,uint32_t path_count,const char *verification);
#endif
#endif
