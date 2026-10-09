#ifndef ENVY_HTML_H
#define ENVY_HTML_H
#include <stdint.h>
int html_find_element(const char *html, const char *id, char *tag, uint32_t capacity);
int html_has_tag(const char *html,const char *tag);
void html_view_begin(const char *title, const char *url);
void html_view_render(const char *html);
void html_view_element(const char *tag, const char *text, const char *css);
void html_view_status(const char *text);
void html_view_image(uint32_t node,const char *source,uint32_t width,uint32_t height);
void html_view_repaint(void);
void html_view_node(uint32_t node, const char *tag, const char *id, const char *classes,
                    const char *text, const char *markup, const char *inline_style);
void html_view_text_lookup(const char *(*lookup)(void *, const char *), void *context);
int html_view_hit_test(int32_t x,int32_t y,char *handler,uint32_t capacity);
#endif
