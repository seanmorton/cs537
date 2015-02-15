// On-disk file system format.
// Both the kernel and user programs use this header file.

// Block 0 is unused.
// Block 1 is super block.
// Inodes start at block 2.

#define ROOTINO 0  // root i-number
#define BSIZE 4096  // block size

#define MFS_BLOCK_SIZE 4096

// File system super block
struct __attribute__((__packed__)) superblock {
  unsigned int size;         // Size of file system image (blocks)
  unsigned int nblocks;      // Number of data blocks
  unsigned int ninodes;      // Number of inodes.
};

#define NDIRECT 13
#define NINDIRECT (BSIZE / sizeof(uint))
#define MAXFILE (NDIRECT + NINDIRECT)

#define MFS_REGULAR_FILE 1
#define MFS_DIRECTORY 2

// On-disk inode structure
struct __attribute__((__packed__)) dinode {
  int type;           // File type
  unsigned int size;            // Size of file (bytes)
  unsigned int addrs[NDIRECT+1];   // Data block addresses
};

// Inodes per block.
#define IPB           (BSIZE / sizeof(struct dinode))

// Block containing inode i
#define IBLOCK(i)     ((i) / IPB + 2)

// Bitmap bits per block
#define BPB           (BSIZE*8)

// Block containing bit for block b
#define BBLOCK(b, ninodes) (b/BPB + (ninodes)/IPB + 3)

typedef struct __MFS_Stat_t {
    int type;   // MFS_DIRECTORY or MFS_REGULAR
    int size;   // bytes
    // note: no permissions, access times, etc.
} MFS_Stat_t;

typedef struct __MFS_DirEnt_t {
    char name[60];  // up to 60 bytes of name in directory (including \0)
    int  inum;      // inode number of entry (-1 means entry not used)
} MFS_DirEnt_t;

// request types          
#define MFS_LOOKUP 101
#define MFS_STAT 102
#define MFS_WRITE 103        
#define MFS_READ 104        
#define MFS_CREAT 105        
#define MFS_UNLINK 106        
#define MFS_SHUTDOWN 107        

typedef struct __attribute__((__packed__)) __message__ {
        int type;
        int file_type;
        int inum;
        int pinum;
        int block;
        char name[64];
        char buffer[BSIZE];
} message;

typedef struct __attribute__((__packed__)) __response__ {
        int retval;
        int inum;
        char msg[16];
        MFS_Stat_t mfs_stat;
        char buffer[BSIZE];
} response;

int MFS_Init(char *hostname, int port);
int MFS_Lookup(int pinum, char *name);
int MFS_Stat(int inum, MFS_Stat_t *m);
int MFS_Write(int inum, char *buffer, int block);
int MFS_Read(int inum, char *buffer, int block);
int MFS_Creat(int pinum, int type, char *name);
int MFS_Unlink(int pinum, char *name);
int MFS_Shutdown(); 

void send_request();
