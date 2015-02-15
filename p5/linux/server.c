/*
 * UDP/MFS server program
 * by Sean Morton
 * TODO check retvals of lib calls
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "udp.h"
#include "mfs.h"

#define BLOCKMASK(blk) (1 << (7 - blk % 8)) 
#define ADDRTOBLOCK(addr) (addr / BSIZE - 4)

message req; // client request
response res; // server response

int mfsfd; // the img file
char meta_blocks[3*BSIZE]; // superblock, inode block, and bitmap
struct superblock *sb = (struct superblock *)&meta_blocks[0*BSIZE];
struct dinode *inodes = (struct dinode *)&meta_blocks[1*BSIZE];
char *bitmap = &meta_blocks[2*BSIZE]; // 1st entry represents 4th block in file system 

/* Reads the bitmap bit for block number  */
int read_bit(int blk) {
  return bitmap[blk/8] & BLOCKMASK(blk); 
}

/* Toggles the bitmap bit for block number */
void toggle_bit(int blk) {
  bitmap[blk/8] ^= BLOCKMASK(blk); 
}

/* Returns the offset (address) of the next free data block */
int get_free_block() {
  int i;
  for (i = 0; i < sb->nblocks; i++) {
    if (read_bit(i) == 0)
      return BSIZE*(i+4); // data blocks start at 4th block */
  }
  return -1;
}

/* Returns the index of a free inode, -1 if none are available */
int get_free_inum() {
  int i;
  for (i = 0; i < sb->ninodes; i++) {
    if (inodes[i].type == 0) 
      return i;
  }
  return -1;
}

/* 
 * Initializes dir_block as a directory block with 
 * the . and .. entries for their respective pinum 
 * and inum values
 */
void initialize_dir(char* dir_block, int pinum, int inum) {
  int offset;

  MFS_DirEnt_t *dirent;
  for (offset = 0; offset < BSIZE; offset += sizeof(MFS_DirEnt_t)) {
    dirent = (MFS_DirEnt_t *)(dir_block+offset);
    if (offset == 0) {
      strncpy(dirent->name, ".", 60);
      dirent->inum = inum;
    } else if (offset == sizeof(MFS_DirEnt_t)) {
      strncpy(dirent->name, "..", 60);
      dirent->inum = pinum;
    } else {
      strncpy(dirent->name, "", 60);
      dirent->inum = -1;
    }
  }
}

/*
 * Creates the file system from scratch with the given filename.
 * Initializes meta blocks and a root directory:
 *
 * 0: unused 
 * 1: superblock
 * 2: inodes
 * 3: data bitmap
 * 4: data blocks start here
 *
 */
void initfs(char* name) {

  mfsfd =  open(name, O_RDWR | O_CREAT, 0666);

  sb->size = 1028;
  sb->nblocks = 1024;
  sb->ninodes = IPB;

  int i, j;
  for (i = 0; i < IPB; i++) {

    struct dinode* inode = &inodes[i];
    inode->type = 0;
    inode->size = 0;

    for (j = 0; j < NDIRECT+1; j++) {
      inode->addrs[j] = ~0;
    }
  }

  memset(bitmap, 0, BSIZE);

  struct dinode* root_inode = &inodes[ROOTINO];
  root_inode->type = MFS_DIRECTORY;
  root_inode->size = 0;
  root_inode->addrs[0] = get_free_block();

  char root_block[BSIZE];
  initialize_dir(root_block, 0, 0); 
  toggle_bit(0);

  pwrite(mfsfd, meta_blocks, 3*BSIZE, 1*BSIZE);
  pwrite(mfsfd, root_block, BSIZE, 4*BSIZE);
}

/*
 * Looks up a directory entry in the directory specified by pinum.
 * If the director entry's name matches the given name, the directory
 * entry's inum is returned.
 */
int m_lookup(int pinum, char *name) {
  if (pinum < 0 || pinum > sb->ninodes-1) {
    sprintf(res.msg, "invalid inode");
    return -1;
  }

  struct dinode dir_inode = inodes[pinum];
  MFS_DirEnt_t *dirent;

  if (dir_inode.type == MFS_REGULAR_FILE || dir_inode.type == 0) {
    sprintf(res.msg, "cannot lookup regular file");
    return -1;
  }
  
  int i;
  char data_block[BSIZE];
  for (i = 0; i < NDIRECT+1; i++) {
    if (dir_inode.addrs[i] == ~0)
      continue;

    lseek(mfsfd, dir_inode.addrs[i], SEEK_SET);
    read(mfsfd, data_block, BSIZE);

    int offset;
    for (offset = 0; offset < BSIZE; offset += sizeof(MFS_DirEnt_t)) {
      dirent = (MFS_DirEnt_t *)(data_block+offset);
      if (dirent->inum != -1) {
        if (strcmp(dirent->name, name) == 0) {
          return dirent->inum;
        }
      }
    }
  }
  sprintf(res.msg, "filename not found");   
  return -1;
}

/*
 * Fills a stat struct with information about the inode specified
 * by inum.
 */
int m_stat(int inum, MFS_Stat_t *m) {
  if (inum < 0 || inum > sb->ninodes-1) {
    sprintf(res.msg, "invalide inode");
    return -1;
  }

  struct dinode* stat_inode = &inodes[inum];
  if (stat_inode->type == 0) {
    sprintf(res.msg, "inode is unused");
    return -1;
  }
  
  m->type = stat_inode->type;
  m->size = stat_inode->size;
  return 0;
}

/* 
 * Writes the contents of buffer to the data block specified by 
 * offset block. On succes, returns 0, -1 on error.
 */
int m_write(int inum, char *buffer, int block) {
  if (inum < 0 || inum > sb->ninodes-1) {
    sprintf(res.msg, "invalid inode");
    return -1;
  }

  if (block < 0 || block > NDIRECT) {
    sprintf(res.msg, "invalid block");
    return -1;
  }

  struct dinode* write_inode = &inodes[inum];
  if (write_inode->type == MFS_DIRECTORY || write_inode->type == 0) {
    sprintf(res.msg, "cannot write to directories");
    return -1;  
  }
  
  if (write_inode->addrs[block] == ~0) { // allocate new data block
    write_inode->addrs[block] = get_free_block();
    toggle_bit(ADDRTOBLOCK(write_inode->addrs[block]));
    pwrite(mfsfd, meta_blocks, 3*BSIZE, 1*BSIZE); // first block is empty
  }

  int i;
  for (i = 0; i < NDIRECT+1; i++) {
    if (write_inode->addrs[i] != ~0) {
      write_inode->size = BSIZE*(block+1);
    }
  }

  lseek(mfsfd, write_inode->addrs[block], SEEK_SET);
  write(mfsfd, buffer, BSIZE);

  return 0;
} 


/* 
 * Reads the contents of block into buffer. 
 * Returns 0 on succes, -1 on error.
 */
int m_read(int inum, char *buffer, int block) {
  if (inum < 0 || inum > sb->ninodes-1) {
    sprintf(res.msg, "invalid inode");
    return -1; 
  }
  
  if (block < 0 || block > NDIRECT) {
    sprintf(res.msg, "invalid block");
    return -1;
  }

  struct dinode read_inode = inodes[inum];
  if (read_inode.type == 0) {
    sprintf(res.msg, "unused inode");
    return -1; 
  }
    
  if (read_inode.addrs[block] == ~0) {
    sprintf(res.msg, "block not allocated");
    return -1;
  }
 
  lseek(mfsfd, read_inode.addrs[block], SEEK_SET);
  read(mfsfd, buffer, BSIZE); 
  return 0;
}

/* 
 * Creates a file in the directory specified by pinum.
 * Returns 0 on succes, -1 on error.
 */
int m_creat(int pinum, int type, char *name) {
  if (pinum < 0 || pinum > sb->ninodes-1) {
    sprintf(res.msg, "invalid inode"); 
    return -1;
  }

  struct dinode parent_inode = inodes[pinum];
  if (parent_inode.type == MFS_REGULAR_FILE || parent_inode.type == 0) {
    sprintf(res.msg, "parent inode is not a directory"); 
    return -1;
  }

  int i;
  char parent_block[BSIZE];
CREAT_START:
  for (i = 0; i < NDIRECT+1; i++) {
    if (parent_inode.addrs[i] == ~0)
      continue;

    lseek(mfsfd, parent_inode.addrs[i], SEEK_SET);
    read(mfsfd, parent_block, BSIZE);

    int offset;
    MFS_DirEnt_t *dirent;
    for (offset = 0; offset < BSIZE; offset += sizeof(MFS_DirEnt_t)) {
      dirent = (MFS_DirEnt_t *)(parent_block+offset);

      if (dirent->inum == -1) { // this dirent is unused

        int new_inum = get_free_inum();
        if (new_inum  == -1) {
          sprintf(res.msg, "no inodes are available");
          return -1;
        }
       
        struct dinode* new_inode;
        new_inode = &inodes[new_inum];
        new_inode->type = type;
        new_inode->size = 0;

        new_inode->addrs[0] = get_free_block();
        toggle_bit(ADDRTOBLOCK(new_inode->addrs[0])); // mark block as alloacted

        pwrite(mfsfd, meta_blocks, 3*BSIZE, 1*BSIZE); // first block is empty
        if (type == MFS_DIRECTORY) { // initialize the directory 
          char dir_block[BSIZE]; 
          initialize_dir(dir_block, pinum, new_inum);
     
          lseek(mfsfd, new_inode->addrs[0], SEEK_SET);
          write(mfsfd, dir_block, BSIZE); 
        } 

        dirent->inum = new_inum;
        strncpy(dirent->name, name, 60);
        
        lseek(mfsfd, parent_inode.addrs[i], SEEK_SET);
        write(mfsfd, parent_block, BSIZE);
      
        return 0; 
      } 
    } 
  }

  // the parent directory has no available entries; so grow it if possbile.
  for (i = 0; i < NDIRECT+1; i++) {
    if (parent_inode.addrs[i] == ~0) {

      parent_inode.addrs[i] = get_free_block();
      toggle_bit(ADDRTOBLOCK(parent_inode.addrs[i]));  
    
      char new_dir_block[BSIZE];
      initialize_dir(new_dir_block, -1, -1); // don't create "." and ".." entries

      lseek(mfsfd, parent_inode.addrs[i], SEEK_SET);
      write(mfsfd, new_dir_block, BSIZE);     

      goto CREAT_START;
    }
  }
 
  sprintf(res.msg, "parent directory is full"); 
  return -1; 
}

/*
 * Removes the file from pinum of the specified name.
 * Directories that are non-empty cannot be removed.
 * Returns 0 on success, -1 on failure.
 */
int m_unlink(int pinum, char *name) {
  if (pinum < 0 || pinum > sb->ninodes-1) {
    sprintf(res.msg, "invalid inode"); 
    return -1;
  }

  struct dinode parent_inode = inodes[pinum];
  if (parent_inode.type == MFS_REGULAR_FILE || parent_inode.type == 0) {
    sprintf(res.msg, "parent inode is not a directory"); 
    return -1;
  }

  int i;
  char parent_block[BSIZE];
  for (i = 0; i < NDIRECT+1; i++) {
    if (parent_inode.addrs[i] == ~0)
      continue;

    lseek(mfsfd, parent_inode.addrs[i], SEEK_SET);
    read(mfsfd, parent_block, BSIZE);

    int offset;
    MFS_DirEnt_t *dirent;
    for (offset = 0; offset < BSIZE; offset += sizeof(MFS_DirEnt_t)) {
      dirent = (MFS_DirEnt_t *)(parent_block+offset);

      if (strcmp(dirent->name, name) == 0) { 

        struct dinode *delete_inode = &inodes[dirent->inum];
        if (delete_inode->type == MFS_DIRECTORY) { // make sure directory is emtpy

          int j;
          char dir_block[BSIZE];
          for (j = 0; j < NDIRECT+1; j++) {
            if (delete_inode->addrs[j] != ~0) {
      
              lseek(mfsfd, delete_inode->addrs[j], SEEK_SET);  
              read(mfsfd, dir_block, BSIZE);
        
              int k;
              MFS_DirEnt_t *tmp;
              for (k = 0; k < BSIZE; k += sizeof(MFS_DirEnt_t)) {
                tmp = (MFS_DirEnt_t *)(dir_block+k);
                if (tmp->inum != -1 && tmp->inum != dirent->inum && tmp->inum != pinum) {
                  sprintf(res.msg, "directory is non-empty");
                  return -1;
                }
              } 
            }
          } 
        }
       
        int m; // deallocate any data blocks associated with the file
        for (m = 0; m < NDIRECT+1; m++) {
          if (delete_inode->addrs[m] != ~0) {
            toggle_bit(ADDRTOBLOCK(delete_inode->addrs[m]));
            delete_inode->addrs[m] == ~0;
          }
        }
        
        delete_inode->type = 0;
        delete_inode->size = 0;
        pwrite(mfsfd, meta_blocks, 3*BSIZE, 1*BSIZE); // first block is empty
 
        strncpy(dirent->name, "", 60);
        dirent->inum = -1;

        lseek(mfsfd, parent_inode.addrs[i], SEEK_SET);
        write(mfsfd, parent_block, BSIZE);

        return 0;
      } 
    } 
  }
  return 0;  // nothing was found to delete
}


int main(int argc, char *argv[]) {
  struct sockaddr_in client;
  int portnum;
    int connfd;

  if (argc != 3) {
    fprintf(stderr, "Usage: server [portnum] [file-system-image]\n");
    exit(1);
  }
  portnum = strtol(argv[1], NULL, 10);
  connfd = UDP_Open(portnum);

  mfsfd = open(argv[2], O_RDWR);
  if (mfsfd == -1) {
    initfs(argv[2]);
    fsync(mfsfd);
  } else {
    pread(mfsfd, meta_blocks, 3*BSIZE, 1*BSIZE); // first block is empty
  }

  for(;;) { 
    
    // reset shared res fields 
    res.retval = -1;

    UDP_Read(connfd, &client, (char*)&req, sizeof(req));
    switch (req.type) {

      case MFS_LOOKUP:
        res.inum = m_lookup(req.pinum, req.name);
        res.retval = 0;
        break;
  
      case MFS_STAT:
        ;
        MFS_Stat_t m;
        res.retval = m_stat(req.inum, &m);
        res.mfs_stat = m;
        break;

      case MFS_WRITE:
        res.retval = m_write(req.inum, req.buffer, req.block);
        fsync(mfsfd);
        break;

      case MFS_READ:
        res.retval = m_read(req.inum, res.buffer, req.block); 
        break;

      case MFS_CREAT:
        if (m_lookup(req.pinum, req.name) > 0) {
          res.retval = 0; // the file has already been created
        } else {
          res.retval = m_creat(req.pinum, req.file_type, req.name);
          fsync(mfsfd);
        }
        break;

      case MFS_UNLINK:
        res.retval = m_unlink(req.pinum, req.name);
        fsync(mfsfd);
        break;

      case MFS_SHUTDOWN:
        fsync(mfsfd);
        close(mfsfd);
        res.retval = 0;
        break;         
 
      default:
        res.retval = -1;
        sprintf(res.msg, "bad request");
    }

    UDP_Write(connfd, &client, (char*)&res, sizeof(res));
    
    if (req.type == MFS_SHUTDOWN) {
      UDP_Close(connfd);
      exit(0);  
    }
  }

  return 0;
}

