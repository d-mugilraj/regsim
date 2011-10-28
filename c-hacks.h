#define ARRAY_SIZE(ar) (sizeof(ar)/sizeof(ar[0]))
#define BUG_ON(cond) do { if (cond) abort(); } while (0)
#define min(a, b) (a < b) ? a : b
