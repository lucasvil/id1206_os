#include <stddef.h>

struct head* dalloc(size_t);
void dfree(void*);
struct head* init();
void printArena();
int getFreeLength();
void sanity();
int getAllocSize();