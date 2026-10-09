#ifndef ENVY_CSS_H
#define ENVY_CSS_H
#include <stdint.h>
enum { CSS_COLOR, CSS_BACKGROUND, CSS_FONT_SIZE, CSS_LINE_HEIGHT, CSS_ALIGN,
    CSS_DISPLAY, CSS_POSITION, CSS_LEFT, CSS_TOP, CSS_RIGHT, CSS_BOTTOM,
    CSS_WIDTH, CSS_HEIGHT, CSS_MARGIN_TOP, CSS_MARGIN_RIGHT, CSS_MARGIN_BOTTOM,
    CSS_MARGIN_LEFT, CSS_PADDING_TOP, CSS_PADDING_RIGHT, CSS_PADDING_BOTTOM,
    CSS_PADDING_LEFT, CSS_BORDER, CSS_BORDER_COLOR, CSS_TRANSLATE_X, CSS_TRANSLATE_Y, CSS_FONT_FAMILY,
    CSS_BACKGROUND_IMAGE, CSS_BACKGROUND_SIZE, CSS_BACKGROUND_X, CSS_BACKGROUND_Y, CSS_BACKGROUND_REPEAT, CSS_Z_INDEX,
    CSS_MIN_WIDTH, CSS_MAX_WIDTH, CSS_MIN_HEIGHT, CSS_MAX_HEIGHT, CSS_BOX_SIZING,
    CSS_FLEX_DIRECTION, CSS_FLEX_WRAP, CSS_JUSTIFY, CSS_ITEMS, CSS_SELF, CSS_GROW, CSS_SHRINK, CSS_BASIS,
    CSS_ROW_GAP, CSS_COLUMN_GAP, CSS_RADIUS, CSS_OPACITY, CSS_VISIBILITY, CSS_OVERFLOW_X, CSS_OVERFLOW_Y,
    CSS_WHITE_SPACE, CSS_FONT_WEIGHT, CSS_COLOR_ALPHA, CSS_BACKGROUND_ALPHA, CSS_BORDER_ALPHA,
    CSS_GRADIENT, CSS_GRID_COLUMNS, CSS_BACKDROP_BLUR, CSS_BACKDROP_BRIGHTNESS, CSS_PROPERTIES };
#define CSS_AUTO (-2147483647)
#define CSS_PERCENT 1000000
enum { CSS_BG_AUTO, CSS_BG_COVER, CSS_BG_CONTAIN, CSS_BG_EXPLICIT };
enum { CSS_NONE, CSS_BLOCK, CSS_FLEX, CSS_INLINE, CSS_INLINE_BLOCK, CSS_GRID };
enum { CSS_START, CSS_CENTER, CSS_END, CSS_BETWEEN, CSS_AROUND, CSS_EVENLY, CSS_STRETCH };
typedef struct { int32_t value[CSS_PROPERTIES]; int hidden; char font_family[256],background_image[256];
    int32_t background_width,background_height,line_height_spec;
    int32_t gradient_angle,gradient_count,gradient_color[8],gradient_alpha[8],gradient_position[8]; } css_style;
typedef struct css_element {
    const char *tag, *id, *classes; const struct css_element *parent;
} css_element;
void css_init(void);
/* Source buffers must remain alive until css_init(). */
int css_add(const char *source, uint32_t length);
int css_add_file(const char *source,uint32_t length,const char *path);
void css_compute(const css_element *element, const css_style *parent,
                 const char *inline_style, uint32_t width, uint32_t height, css_style *result);
int32_t css_resolve(int32_t value, int32_t basis);
void css_state_lookup(int (*lookup)(const css_element *,const char *));
int css_selector_matches(const css_element *element,const char *selector);
#endif
