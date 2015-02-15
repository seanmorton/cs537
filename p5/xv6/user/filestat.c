#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"

int
main(int argc, char *argv[])
{
  int fd;
  struct stat st;
 
  if(argc != 2){
    printf(1, "error: no path specified\n");
  }

  fd = open(argv[1], 0);
  if (fd == -1)
    exit();

  fstat(fd, &st);
  printf(1, "Type: %d\n", st.type);
  printf(1, "Device Num: %d\n", st.dev);
  printf(1, "Inode: %d\n", st.ino);
  printf(1, "Logical Size: %d\n", st.logical_size);  
  printf(1, "Physical Size: %d\n", st.physical_size);

  exit();
}
