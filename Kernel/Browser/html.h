#ifndef ENVY_HTML_H
#define ENVY_HTML_H
#include <stdint.h>
struct css_element;
int html_find_element(const char *html, const char *id, char *tag, uint32_t capacity);
int html_has_tag(const char *html,const char *tag);
void html_query_selector(const char *html,const char *selector,const struct css_element *root,void *context,
    uint32_t (*materialize)(void *,uint32_t,uint32_t,const char *,const char *,const char *,const char *),
    void (*selected)(void *,uint32_t));
void html_view_begin(const char *title, const char *url);
void html_view_render(const char *html);
void html_view_element(const char *tag, const char *text, const char *css);
void html_view_status(const char *text);
void html_view_image(uint32_t node,const char *source,uint32_t width,uint32_t height);
void html_view_repaint(void);
void html_view_batch_begin(void);
void html_view_batch_end(void);
uint32_t html_view_paint_count(void);
void html_view_node(uint32_t node, const char *tag, const char *id, const char *classes,
                    const char *text, const char *markup, const char *inline_style);
void html_view_text_lookup(const char *(*lookup)(void *, const char *), void *context);
void html_view_style_lookup(void (*lookup)(void *,uint32_t,const char *,char *,uint32_t),void *context);
void html_view_markup_lookup(const char *(*lookup)(void *,const char *),void *context);
void html_view_parent(uint32_t node,const char *parent_id);
int html_view_pointer(int32_t x,int32_t y,uint8_t buttons);
int html_view_scroll(int32_t x,int32_t y,int32_t dx,int32_t dy);
const char *html_view_target(int32_t x,int32_t y);
uint32_t html_view_target_node(int32_t x,int32_t y);
int html_view_bounds(uint32_t node,const char *id,int32_t *x,int32_t *y,int32_t *w,int32_t *h);
int html_view_geometry(uint32_t node,const char *id,int32_t *x,int32_t *y,int32_t *w,int32_t *h);
int html_view_hit_test(int32_t x,int32_t y,char *handler,uint32_t capacity);
#endif
