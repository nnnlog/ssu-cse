#include <stdio.h>

#if DEBUG == 1
#define debug_print(...) printf("[DEBUG] "__VA_ARGS__)
#else
#define debug_print(...)
#endif

extern void debug_hash(const unsigned char *str, int length);
