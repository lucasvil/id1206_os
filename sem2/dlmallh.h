struct head;
void* dalloc(size_t);
void dfree(void*);
struct head* init();
void printArena();
int getFreeLength();
void sanity();
int getAllocSize();