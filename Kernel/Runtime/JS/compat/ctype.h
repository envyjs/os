#ifndef ENVY_CTYPE_H
#define ENVY_CTYPE_H
static inline int isspace(int c){return c==32||(c>=9&&c<=13);}
static inline int isdigit(int c){return c>=48&&c<=57;}
#endif
