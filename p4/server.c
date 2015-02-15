// 
// server.c: A very, very simple web server
//
// To run:
//  server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//

#include <pthread.h>
#include "cs537.h"
#include "request.h"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t full = PTHREAD_COND_INITIALIZER;
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;

int *buffer;
int max = 0;
int fill = 0;
int use = 0;
int count = 0;

// CS537: Parse the new arguments too
void getargs(int *port, int *threads, int *buffers, int argc, char *argv[])
{
    if (argc != 4) { 
	    fprintf(stderr, "Usage: %s <port> <threads> <buffers>\n", argv[0]);
	    exit(1);
    }
    
    *port = atoi(argv[1]);
    *threads = atoi(argv[2]);
    *buffers = atoi(argv[3]); 
    
    if (*port < 2000) {
      fprintf(stderr, "error: port must be >= 2000");
      exit(1);
    }

    if (*threads < 1) {
      fprintf(stderr, "error: threads must be > 0");
      exit(1);
    } 
    
    if (*buffers < 1) {
      fprintf(stderr, "error: buffers must be > 0");
      exit(1);
    }
}

void put(int value) {
  buffer[fill] = value;
  fill = (fill + 1) % max;
  count++;
}

int get() {
  int tmp = buffer[use];
  use = (use + 1) % max;
  count--;
  return tmp;
}

void producer (int connfd) {
  pthread_mutex_lock(&mutex);
  while (count == max) 
    pthread_cond_wait(&empty, &mutex);
  put(connfd);
  pthread_cond_signal(&full);
  pthread_mutex_unlock(&mutex); 
}

void *consumer (void *arg) { 
  while (1) { 
  pthread_mutex_lock(&mutex);
  while (count == 0) 
    pthread_cond_wait(&full, &mutex);
  int fd = get();
  pthread_cond_signal(&empty);
  pthread_mutex_unlock(&mutex); 
  requestHandle(fd);
  Close(fd);
  } 
}

int main(int argc, char *argv[])
{
    int listenfd, connfd, port, clientlen;
    int threads, buffers;
    struct sockaddr_in clientaddr;

    getargs(&port, &threads, &buffers, argc, argv );
    
    int i;
    for (i = 0; i < threads; i++) {
      pthread_t worker;
      pthread_create(&worker, NULL, consumer, NULL); 
    }
    
    max = buffers;
    buffer = (int *)malloc(sizeof(int)*buffers);

    listenfd = Open_listenfd(port);
    while (1) {
	    clientlen = sizeof(clientaddr);
	    connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
	    // 
	    // CS537: In general, don't handle the request in the main thread.
	    // Save the relevant info in a buffer and have one of the worker threads 
	    // do the work.
      producer(connfd);
    }
}
