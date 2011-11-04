#include <stdlib.h>

#define ARRAY_SIZE(ar) (sizeof(ar)/sizeof(ar[0]))
#define BUG_ON(cond) do { if (cond) abort(); } while (0)
#define min(a, b) (a < b) ? a : b

#define isupper(c) (c >= 0x41 && c <= 0x5A) ? true: false
#define islower(c) (c >= 0x61 && c <= 0x7A) ? true: false
#define isalpha(c) (isupper(c) || islower(c)) ? true: false
#define toupper(c) (islower(c)) ? c - 'a'-'A' : c
