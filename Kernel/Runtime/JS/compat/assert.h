#ifndef ENVY_JS_ASSERT_H
#define ENVY_JS_ASSERT_H
#include <stdlib.h>
#define assert(x) ((x)?(void)0:envy_js_abort())
#endif
