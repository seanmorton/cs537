/*
 * MFS client library
 * by Sean Morton
 */ 

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "udp.h"
#include "mfs.h"

struct sockaddr_in server;
struct sockaddr_in other;
int connfd;
int init_called = 0; // false if MFS_Init() has not been called 
message req; // request to server
response res; // response from server

int MFS_Init(char *hostname, int port) {
  UDP_FillSockAddr(&server, hostname, port);
  connfd = UDP_Open(0);
  init_called = 1;

  return 0;
}

int MFS_Lookup(int pinum, char *name) {
  if (!init_called)
    return -1;

  req.type = MFS_LOOKUP; 
  req.pinum = pinum;
  strncpy(req.name, name, sizeof(req.name));

  send_request();
  return res.inum;
}

int MFS_Stat(int inum, MFS_Stat_t *m) {
  if (!init_called)
    return -1;

  req.type = MFS_STAT;
  req.inum = inum;

  send_request();

  m->type = res.mfs_stat.type;
  m->size = res.mfs_stat.size;
  return res.retval;
}

int MFS_Write(int inum, char *buffer, int block) {
  if (!init_called)
    return -1;

  req.type = MFS_WRITE;
  req.inum = inum;
  req.block = block;
  memcpy(req.buffer, buffer, sizeof(req.buffer));
  
  send_request();
  return res.retval;;
}

int MFS_Read(int inum, char *buffer, int block) {
  if (!init_called)
    return -1;

  req.type = MFS_READ;
  req.inum = inum;
  req.block = block;

  send_request();
  memcpy(buffer, res.buffer, BSIZE);
  return res.retval;
}

int MFS_Creat(int pinum, int type, char *name) {
  if (!init_called)
    return -1;

  if (strlen(name) > 60) 
    return -1;

  req.type = MFS_CREAT;
  req.pinum = pinum;
  req.file_type = type;
  strncpy(req.name, name, sizeof(req.name));

  send_request();
  return res.retval;
}

int MFS_Unlink(int pinum, char *name) {
  if (!init_called)
    return -1;

  req.type = MFS_UNLINK;
  req.pinum = pinum;
  strncpy(req.name, name, sizeof(req.name));

  send_request();
  return res.retval;
}

int MFS_Shutdown() {
  if (!init_called) 
    return -1;

  req.type = MFS_SHUTDOWN;
  send_request();

  UDP_Close(connfd);
  return 0;
}

// Send a request to the server; on timeout, try again
void send_request() {
  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(connfd, &fds);
  struct timeval tv;

  tv.tv_sec = 3;
  tv.tv_usec = 0;

  UDP_Write(connfd, &server, (char*)&req, sizeof(req));
  
  int ready = select(connfd+1, &fds, NULL, NULL, &tv);
  if (ready == 1) { 
    UDP_Read(connfd, &other, (char*)&res, sizeof(res));
    if (res.retval == -1)
      printf("Request failed because %s\n", res.msg);
  } else {
    printf("Request timed out. trying again..\n");
    send_request();
  }
}
