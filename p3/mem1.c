/* 
 * Workload 1: assume all blocks are 16 bytes - use a memory pool
 * Author: Sean Morton
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <stdint.h>
#include "mem.h"

#define BLOCKSIZE 16 

typedef struct block_header{
  struct block_header* next;
}block_header;

char* start;        // The start of the memory space from Mem_Init()
char* end;          // The end of the memory space from Mem_Init()
block_header* head; // The head of the free list
int num_blocks;     // The number of blocks that can fit into the space from Mem_Init()
int free_space;     // The amount of free space available 

int Mem_Init(int size) {
  static int already_called = 0; // this function can only be called once
  int pagesize;
  int extra_bytes;
  int total_size;
  int fd;

  if(already_called || size <= 0) {
    return -1;
  }

  pagesize = getpagesize();

  // Ensure the allocated block is in units of page size 
  extra_bytes = size % pagesize;
  extra_bytes = (pagesize - extra_bytes) % pagesize;

  total_size = size + extra_bytes;
  free_space = total_size;
  num_blocks = total_size / BLOCKSIZE;

  fd = open("/dev/zero", O_RDWR);
  if(fd == -1) {
    return -1;
  }
  
  start = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
  if (start == MAP_FAILED) {
    return -1;
  }
  end = start + total_size;
 
  head = (block_header*)start;
  
  // Build the memory pool (a linked list with fixed sized segments)
  int i;
  block_header* curr = head;
  for (i = 1; i < num_blocks; i++) {
    block_header* new_block = (block_header*) ((char*) curr + BLOCKSIZE);
    curr->next = new_block;
    curr = new_block;
  }
  curr->next = NULL;
  
  already_called = 1; 
  close(fd);
  return 0;
}

void* Mem_Alloc(int size) {

  if (size != 16 || free_space < 48 ) {
    return NULL;
  }

  void* user_ptr; 
 
  // Give the user the first block in the free list 
  user_ptr = (char*) head;
  head = head->next; 
  free_space -= BLOCKSIZE;
  
  return user_ptr;
}

int Mem_Free(void *ptr) {   
  
  if (ptr == NULL) {
    return -1;
  }

  // Ensure pointer points to something that was alloacted by Mem_Alloc()
  if ((char*)ptr < start && (char*)ptr > end) {
    return -1; // ptr was not in the address space given by Mem_Init()
  }
  if ((uintptr_t) ptr % BLOCKSIZE) { 
    return -1; // ptr did not point to the beginning of a block 
  }
  block_header* curr = head;
  while (curr != NULL) {
    if ((char*) curr == ptr) {
      return -1; // trying to free an already free block
    }
    curr = curr->next;
  }

  // Add this block to the front of the free list
  block_header* new_free_block = (block_header*) ptr;
  new_free_block->next = head;
  head = new_free_block; 

  // Mark this block as free 
  ptr = NULL;
  free_space += BLOCKSIZE;
 
  return 0;
}

int Mem_Available() {
  return free_space;
}

void Mem_Dump() {
  block_header* curr_node= NULL;
  char* begin_addr = NULL;
  char* end_addr = NULL;
  curr_node = head;
  int counter = 0;

  printf("start_addr\tend_addr\t\n");
  while(curr_node != NULL) {
    begin_addr = (char*)curr_node;
    end_addr  = begin_addr + BLOCKSIZE -1;
    printf("%p\t%p\t\n",begin_addr,end_addr);
    curr_node = curr_node->next;
    counter++;
  }
  printf("%d\n", counter);
  printf("%p -> %p\n", start, end);
  return;
}
