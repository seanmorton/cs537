/*
 * Workload 2: Blocks are allocated in sizes of 16, 80, or 256 bytes
 * Author: Sean Morton
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include "mem.h"

typedef struct block_header{
  int size; // this is the size of the allocated area, excluding the header
  int in_use; // true if this block is not free 
  int wasted; // to ensure 8-byte alignment 
  struct block_header* next;

}block_header;

block_header* head = NULL;

int Mem_Init(int size) {
  static int already_called = 0; // this function can only be called once
  int pagesize;
  int extra_bytes;
  int total_size;
  int fd;
  void* init_ptr;

  if(already_called || size <= 0) {
    return -1;
  }

  pagesize = getpagesize();

  // Ensure the allocated block is in units of page size 
  extra_bytes = size % pagesize;
  extra_bytes = (pagesize - extra_bytes) % pagesize;

  total_size = size + extra_bytes;

  fd = open("/dev/zero", O_RDWR);
  if(fd == -1) {
    return -1;
  }
  
  init_ptr = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
  if (init_ptr == MAP_FAILED) {
    return -1;
  }
  
  head = (block_header*)init_ptr;
  head->next = NULL;
  head->size = total_size - (int)sizeof(block_header);
  head->in_use = 0;
  
  already_called = 1; 
  close(fd);
  return 0;
}

void* Mem_Alloc(int size) {
  block_header* curr_node = NULL;
  block_header* new_block = NULL; // the result of a split
  int orig_size;
  int extra_bytes; 
  int rounded_size;
  void* block_ptr; 
  
  if (size <= 0) { 
    return NULL;
  }
 
  // Ensure the returned pointer is 8-byte aligned
  extra_bytes = size % 8;
  extra_bytes = (8 - extra_bytes) % 8;
  rounded_size = size + extra_bytes; 
  
  // This implementation uses the first fit policy
  curr_node = head;
  while (curr_node != NULL) {
      
    if (curr_node->in_use || curr_node->size <= rounded_size) {
      curr_node = curr_node->next;
    } else { 

      // This block is the first fit 
      orig_size = curr_node->size;
      curr_node->size = rounded_size;
      curr_node->in_use = 1;
      block_ptr = (char*) curr_node + (int)sizeof(block_header);
        
      // Split into two blocks if possible
      if (orig_size - rounded_size >= (int)sizeof(block_header) + 8) {
        new_block = (block_header*) ((char*) curr_node + (int)sizeof(block_header) + rounded_size);
        new_block->size = orig_size - rounded_size - (int)sizeof(block_header);
        new_block->in_use = 0;

        if (curr_node->next != NULL) {
          new_block->next = curr_node->next;
        } else {
          new_block->next = NULL;
        }
        curr_node->next = new_block;
      } 
      return block_ptr;
    }
  }
  return NULL;
}

int Mem_Free(void *ptr) {   
  block_header* ptr_block = NULL; // The block that ptr supposedly points to 
  block_header* free_node = NULL; // The block that ptr is indeed referencing 

  // Used for coalescing
  block_header* prev_block = NULL; // The block directly before the block being freed 
  block_header* next_block = NULL; // The block directly after the block being freed 
  int prev_free = 0; // True if the previous block is free 
  int next_free = 0; // True if the next block is free
  
  if (ptr == NULL) {
    return -1;
  }

  // Ensure pointer points to something that was alloacted by Mem_Alloc() 
  free_node = head;
  ptr_block = (block_header*) ((char*) ptr - (int)sizeof(block_header));
  while (free_node != NULL) {
    if (ptr_block == free_node && free_node->in_use) {
      break; // pointer was indeed initialized by Mem_Alloc()
    } else {
      free_node = free_node->next;
      if (free_node == NULL) {
        return -1; // there is no block that ptr was pointing to
      }
    }
  } 

  // Mark this block as free 
  free_node->in_use = 0;
  ptr = NULL;
 
  // Coalesce: First get the blocks adjacent to the freed block 
  if (free_node == head) {
    prev_block = NULL; // head has no previous block 
  } else {
    prev_block = head;
    while (prev_block->next != free_node) {
      prev_block = prev_block->next;
    }
  }    
  next_block = free_node->next; 

  // Check if adjacent blocks are free 
  if (prev_block != NULL && !(prev_block->in_use)) {
    prev_free = 1;
  }
  if (next_block != NULL && !(next_block->in_use)) {
    next_free = 1;
  }

  if (prev_free && !next_free) {
    prev_block->size += (free_node->size + (int)sizeof(block_header));
    prev_block->next = free_node->next;
    return 0;
  }
  
  if (!prev_free && next_free) {
    free_node->size += (next_block->size + (int)sizeof(block_header));
    free_node->next = next_block->next;
    return 0;
  }

  if (prev_free && next_free) {
    prev_block->size += free_node->size + next_block->size + (2 * (int)sizeof(block_header));
    prev_block->next = next_block->next;
  }
  return 0;
}

int Mem_Available() {
  block_header* curr_node = head;
  int total_available = 0;

  while (curr_node != NULL) {
    if (!(curr_node->in_use)) {
      total_available += curr_node->size;
    }
    curr_node = curr_node->next;      
  }
  printf("%d\n", total_available);
  return total_available;
}

void Mem_Dump() {
  block_header* curr_node= NULL;
  int status;
  char* begin_addr = NULL;
  char* end_addr = NULL;
  int size;

  curr_node = head;
  printf("status\tstart_addr\tend_addr\tsize\t\n");
  while(curr_node != NULL) {
    begin_addr = (char*)curr_node + (int)sizeof(block_header);
    size = curr_node->size;
    status = curr_node->in_use;
    end_addr  = begin_addr + size;
    printf("%d\t%p\t%p\t%d\t\n",status,begin_addr,end_addr,size);
    curr_node = curr_node->next;
  }
  return;
}
