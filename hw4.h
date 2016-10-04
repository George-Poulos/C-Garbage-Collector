#include <unistd.h>

typedef struct chunk {
	size_t size;
	struct chunk* next;
	struct chunk* prev;
} chunk;


void init_gc();
void gc();
