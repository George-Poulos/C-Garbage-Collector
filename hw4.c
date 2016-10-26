#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <malloc.h>

struct memory_region{
  size_t * start;
  size_t * end;
};

struct memory_region global_mem;
struct memory_region heap_mem;
struct memory_region stack_mem;

void walk_region_and_mark(void* start, void* end);

//how many ptrs into the heap we have
#define INDEX_SIZE 1000
void* heapindex[INDEX_SIZE];


//grabbing the address and size of the global memory region from proc 
void init_global_range(){
  char file[100];
  char * line=NULL;
  size_t n=0;
  size_t read_bytes=0;
  size_t start, end;

  sprintf(file, "/proc/%d/maps", getpid());
  FILE * mapfile  = fopen(file, "r");
  if (mapfile==NULL){
    perror("opening maps file failed\n");
    exit(-1);
  }

  int counter=0;
  while ((read_bytes = getline(&line, &n, mapfile)) != -1) {
    if (strstr(line, "hw4")!=NULL){
      ++counter;
      if (counter==3){
        sscanf(line, "%lx-%lx", &start, &end);
        global_mem.start=(size_t*)start;
        // with a regular address space, our globals spill over into the heap
        global_mem.end=malloc(256);
        free(global_mem.end);
      }
    }
    else if (read_bytes > 0 && counter==3) {
      if(strstr(line,"heap")==NULL) {
        // with a randomized address space, our globals spill over into an unnamed segment directly following the globals
        sscanf(line, "%lx-%lx", &start, &end);
        printf("found an extra segment, ending at %zx\n",end);						
        global_mem.end=(size_t*)end;
      }
      break;
    }
  }
  fclose(mapfile);
}


//marking related operations

int is_marked(size_t* chunk) {
  return ((*chunk) & 0x2) > 0;
}

void mark(size_t* chunk) {
  (*chunk)|=0x2;
}

void clear_mark(size_t* chunk) {
  (*chunk)&=(~0x2);
}

// chunk related operations

#define chunk_size(c)  ((*((size_t*)c))& ~(size_t)3 ) 
void* next_chunk(void* c) { 
  if(chunk_size(c) == 0) {
    printf("Panic, chunk is of zero size.\n");
  }
  if((c+chunk_size(c)) < sbrk(0))
    return ((void*)c+chunk_size(c));
  else 
    return 0;
}
int in_use(void *c) { 
  return (next_chunk(c) && ((*(size_t*)next_chunk(c)) & 1));
}


// index related operations

#define IND_INTERVAL ((sbrk(0) - (void*)(heap_mem.start - 1)) / INDEX_SIZE)
void build_heap_index() {
  // TODO
}

//determine if what "looks" like a pointer actually points to a block in the heap
size_t * is_pointer(size_t * ptr) {
	if (!ptr) {
        return NULL;
    }
    // first check whether it's in range of heap memory (exclude last block)
    if (ptr < heap_mem.start || ptr >= heap_mem.end) {
        return NULL;
    }

    // find the header for this chunk
    // traverse entire heap and find the header for this chunk
    size_t* current_mem = heap_mem.start;  // points to mem section of current chunk
    while (current_mem < heap_mem.end) {
        size_t* current_chunk = current_mem-2;  // points to header section of current chunk
        // now check if the pointer in question is between current and next chunk
        if(current_chunk < sbrk(0)) return NULL;
	size_t* next_mem = next_chunk(current_chunk) + 2;
        if (current_mem <= ptr && ptr < next_mem)
            return current_chunk;  // return header to this chunk
        
        current_mem = next_mem;  // move on to next chunk
    }

    return NULL;
}

int chunkAllocated(size_t* b) {
    size_t* nextchunk = next_chunk(b);
    // avoid segfaults by checking if the given pointer is within heap
    if (nextchunk < heap_mem.start || nextchunk >= heap_mem.end)
        return 0;
    // the least sig. bit of next_chunk+1 has current chunk allocated bit
    return (long)(*(nextchunk + 1)) & 1;
}

// the actual collection code
void sweep() {
	/*size_t *current_mem =  heap_mem.start;
	size_t *end = heap_mem.end;
	while (current_mem < end) {

        size_t* current_chunk = current_mem-1;  // points to header section of current chunk
       	if((void *)current_chunk < sbrk(0))return;
        // now check if the pointer in question is between current and next chunk
        size_t* next_mem = next_chunk(current_chunk) + 1;
        // if current chunk is marked, unmark it so we reset for the next gc() call 
        if (is_marked(current_chunk)) {
            clear_mark(current_chunk);
        // if current chunk is unmarked AND allocated, then we can free it (give it mem pointer)
        } else{ 
            free(current_mem);
        }
        
        current_mem = next_mem;  // move on to next chunk
        end = sbrk(0);  // update heap end in case the OS shrinks it after a free
    }	*/
}

long length(size_t* b) {

    // b-1 gives the chunk size, we need to remove lower three bits cuz flags
    return (long)(*(b + 1)) >> 3;  // return size in words (8 bytes each)
}

void rec_mark (size_t *current_chunk){
	if (!current_chunk)
        return;
    if (is_marked(current_chunk))
        return;
    mark(current_chunk);
    // len is length of entire chunk minus header
    int len = length(current_chunk) - 1;
    size_t* current_mem = current_chunk + 1;
    // now call this recursively on every block in this chunk (within mem user data)
    for (int i=0; i < len; i++) {
        size_t* nextchunk = is_pointer((size_t*)*(current_mem + i));
        rec_mark(nextchunk);
    }
}

void walk_region_and_mark(void* start, void* end) {
    for (size_t* current_global = (size_t*)start; (void *)current_global < end; current_global++) {
        size_t* current_chunk = is_pointer(current_global);

        // if not NULL and we're not looping inside of the heap, then mark it recursively
        if (current_chunk && ((void *)current_global < start || (void *)current_global > end)) {
        	rec_mark(current_chunk);        
    	}
    }
}

// standard initialization 

void init_gc() {
  size_t stack_var;
  init_global_range();
  heap_mem.start=malloc(512);
  //since the heap grows down, the end is found first
  stack_mem.end=(size_t *)&stack_var;
}

void gc() {
  size_t stack_var;
  heap_mem.end=sbrk(0);
  //grows down, so start is a lower address
  stack_mem.start=(size_t *)&stack_var;

  // build the index that makes determining valid ptrs easier
  // implementing this smart function for collecting garbage can get bonus;
  // if you can't figure it out, just comment out this function.
  // walk_region_and_mark and sweep are enough for this project.
  build_heap_index();

  //walk memory regions
  walk_region_and_mark(global_mem.start,global_mem.end);
  walk_region_and_mark(stack_mem.start,stack_mem.end);
  sweep();
}
