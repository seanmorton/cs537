/*
 * UDP client program
 * by Sean Morton
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "udp.h"
#include "mfs.h"

void gen_block(char* block, int num) {
  char start[3];
  char end[4];

  start[0] = 's';
  start[1] = 't';
  start[2] = 'r';

  end[0] = 'e';
  end[1] = 'n';
  end[2] = 'd';

  int i;
  for (i = 0; i < 3; i++) 
    block[i] = start[i];
  for (i = 0; i < 3; i++)
    block[BSIZE-(i+1)] = end[3-(i+1)];
}

int main(int argc, char *argv[]) {

  MFS_Init("localhost", 3000);
  printf("MFS_Creat returned %d\n", MFS_Creat(0, MFS_REGULAR_FILE, "test"));
  printf("MFS_Lookup returned %d\n", MFS_Lookup(0, "test"));
  char buf1[BSIZE];
  gen_block(buf1, 1);
  char buf2[BSIZE];
  printf("buf1[4095] = %c\n", buf1[4095]);
  printf("MFS_Write returned %d\n", MFS_Write(1, buf1, 0));
  printf("MFS_Read returned %d\n", MFS_Read(1, buf2, 0));
  printf("memcmp returns %d\n", memcmp(buf1, buf2, BSIZE));
  MFS_Shutdown();

  return 0;
}
