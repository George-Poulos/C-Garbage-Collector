#include "hw4.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#define MAX_ALLOCATIONS 10000
void* allocs[MAX_ALLOCATIONS];
chunk* bottom;

int timediff() {
	static struct timeval before;
	struct timeval now;
	gettimeofday(&now,0);
	int diff = (now.tv_sec - before.tv_sec)*1000 + (now.tv_usec - before.tv_usec)/1000;
	before = now;
	return diff;
}

void *pre;
size_t heap_size() {
	void *ptr = malloc(10000);
	free(ptr);
	return ptr-pre;
}

int free_chunks() {
	size_t* next = (size_t*)bottom;
	int free_chunks=0;
	int prev_size = 0;
	while((void*)next<sbrk(0)) {		
		size_t size = *next & ~1;
		int prev_inuse = *next & 1;
		
		if(next!=(size_t*)bottom) {
			if(!prev_inuse) 
				free_chunks++;
		}
		prev_size = size;
		next = (void*)next+size;
	}
	return free_chunks;
}

int inuse_chunks() {
	size_t* next = (size_t*)bottom;
	int inuse_chunks=0;
	int prev_size = 0;
	while((void*)next<sbrk(0)) {		
		size_t size = *next & ~1;
		int prev_inuse = *next & 1;
		
		if(next!=(size_t*)bottom) {
			if(prev_inuse)
				inuse_chunks++;
		}
		prev_size = size;
		next = (void*)next+size;
	}
	return inuse_chunks;
}

/* this keeps pointers strictly on the stack, so at i==50, we'll have allocated 100 chunks, and gc'd... 49? */
void* recursive_allocations(int i) {
	void* ptr = malloc(i*100+128);
	if(i==0) return ptr;

	void *ptr2 = recursive_allocations(i-1); 
	if(i==50) {
		gc();
		printf("Recursive1: at depth 50, %zu, free %d, inuse %d\n",heap_size(),free_chunks(),inuse_chunks());
	}
	return ptr;	
}

/* here the returned pointer is stored in our local allocation before we return. Hence at depth 50, we're not
	 able to GC anything. */
void* recursive_allocations2(int i) {
	void** ptr = malloc(i*100+128);
	if(i==0) return ptr;

	*ptr = recursive_allocations2(i-1);
	if(i==50) {
		gc();
		printf("Recursive2: At depth 50, %zu, free %d, inuse %d\n",heap_size(),free_chunks(),inuse_chunks());
	}
	return ptr;	
}

int random_up_to(int up_to) {
	return random()%up_to;
}
		 
int main(int argc, char** argv) {
	pre=sbrk(0);
	bottom=malloc(256)-sizeof(size_t); // don't free this one - that keeps it an innocuous stub (a root with no graph attached)
 
	init_gc();

	timediff();
		 
	printf("Checking global root set handling an general GC functionality\n");

	/* the most basic allocation and clearing pointer exercise. This only checks for following the root set pointers one level. */
	void *pre = sbrk(0);
	for(int i=0;i<MAX_ALLOCATIONS;i++) 
		allocs[i]=malloc(i*2+128); 		 

	printf("Heap after first round of allocations: %zu, free %d, inuse %d\n",heap_size(),free_chunks(),inuse_chunks());

	for(int i=0;i<MAX_ALLOCATIONS;i++) 
		allocs[i]=0; 		 
	gc();
	printf("Heap after first gc(): %zu, free %d, inuse %d\n",heap_size(),free_chunks(),inuse_chunks());

	/* allocations which all point to each other. this checks for proper traversal of the chunk graph. */
	for(int i=0;i<MAX_ALLOCATIONS;i++) {
		allocs[i]=malloc(i*2+128); 
		if(i>0)
			*(void**)(allocs[i])=allocs[i-1];
	}
	printf("Heap after second round of allocations: %zu, free %d, inuse %d\n",heap_size(),free_chunks(),inuse_chunks());
	for(int i=0;i<MAX_ALLOCATIONS-1;i++) 
		allocs[i]=0;
	gc();
	// here, since we keep the last entry, which points to the next-to-last and so on, everything should still be around
	printf("Heap after clearing all but one, and gc(): %zu, free %d, inuse %d\n",heap_size(),free_chunks(),inuse_chunks());

	allocs[MAX_ALLOCATIONS-1]=0;
	gc();
	printf("Heap after clearing last one, and gc(): %zu, free %d, inuse %d\n",heap_size(),free_chunks(),inuse_chunks());


	/* allocations which all point to each other. this checks for proper traversal of the chunk graph. */
	for(int i=0;i<MAX_ALLOCATIONS;i++) {
		allocs[i]=malloc(i*2+128); 
		if(i>0) {
			void *start_of_new_alloc = allocs[i];
			void *start_of_prev_alloc = allocs[i-1];
			
			int offset_into_new_alloc = 8*random_up_to((i*2+120)/8);
			int offset_into_old_alloc = 8*random_up_to(((i-1)*2+120)/8);
			void **location_of_pointer = (void**)(start_of_new_alloc + offset_into_new_alloc);

			*location_of_pointer = start_of_prev_alloc + offset_into_old_alloc;
		}
	}
	printf("Heap after third round of allocations: %zu, free %d, inuse %d\n",heap_size(),free_chunks(),inuse_chunks());
	for(int i=0;i<MAX_ALLOCATIONS-1;i++) 
		allocs[i]=0;
	gc();
	// here, since we keep the last entry, which points to the next-to-last and so on, everything should still be around
	printf("Heap after clearing all but one, and gc(): %zu, free %d, inuse %d\n",heap_size(),free_chunks(),inuse_chunks());

	allocs[MAX_ALLOCATIONS-1]=0;
	gc();
	printf("Heap after clearing last one, and gc(): %zu, free %d, inuse %d\n",heap_size(),free_chunks(),inuse_chunks());

	printf("Now checking stack root set handling.\n");

	recursive_allocations(100);
	gc();
	printf("After Recursive1 %zu, free %d, inuse %d\n",heap_size(),free_chunks(),inuse_chunks());
		 
	recursive_allocations2(100);	 
	gc();
	printf("After Recursive2 %zu, free %d, inuse %d\n",heap_size(),free_chunks(),inuse_chunks());
		 
}
