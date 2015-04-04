/* A simple echo server using TCP */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include "Request.h"
#include "SuperFastHash.h"
#include "Cache.h"
#include <pthread.h>

#define SERVER_TCP_PORT 8080

// Usage:  ./proxyFilter [port] [blacklist-path]
int main(int argc, char **argv) {
  int sd, port;
  struct sockaddr_in proxy;

  if ( argc == 3 ){
    port = atoi(argv[1]);
  } else {
    fprintf(stderr, "Usage: %s [port] [blacklist-path]\n", argv[0]);
    exit(1);
  }

  /* Initialize Cache Directory */
  system ("pwd");
  init_cache_dir();

  /* Create a stream socket. */
  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    fprintf(stderr, "Can't create a socket.\n");
    exit(1);
  }

  /* Bind an address to the socket */
  memset((char *) &proxy, 0, sizeof(struct sockaddr_in));
  proxy.sin_family = AF_INET;
  proxy.sin_port = htons(port);
  proxy.sin_addr.s_addr = htonl(INADDR_ANY);
  if (bind(sd, (struct sockaddr *)&proxy, sizeof(proxy)) ==-1) {
    fprintf(stderr, "Can't bind name to socket.\n");
    exit(1);
  }

  /* Receive from the client. */
  listen(sd, 4);

  /* Open BlackList */
  FILE* blacklist;
  blacklist = fopen(argv[2], "r");

  /* Check if File Exists */
  if (!blacklist){
      printf("Error, blacklist does not exist at:  %s", argv[2]);
      exit(1);
  }

  /* Initialize 4 Threads to Accept on Socket */
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_t req_1,req_2,req_3,req_4;
  Thread_Args args1, args2, args3, args4;
  args1.socket_descriptor = sd;
  args1.blacklist = blacklist;
  args2.socket_descriptor = sd;
  args2.blacklist = blacklist;
  args3.socket_descriptor = sd;
  args3.blacklist = blacklist;
  args4.socket_descriptor = sd;
  args4.blacklist = blacklist;

  /* Create 4 Threads to Process Client Requests */
  if(pthread_create(&req_1, NULL, process_request, &args1)) {
    fprintf(stderr, "Error creating thread\n");
    return 1;
  }

  if(pthread_create(&req_2, NULL, process_request, &args2)) {
    fprintf(stderr, "Error creating thread\n");
    return 1;
  }

  if(pthread_create(&req_3, NULL, process_request, &args3)) {
    fprintf(stderr, "Error creating thread\n");
    return 1;
  }

  if(pthread_create(&req_4, NULL, process_request, &args4)) {
    fprintf(stderr, "Error creating thread\n");
    return 1;
  }

  if(pthread_join(req_1, NULL)) {
    fprintf(stderr, "Error joining thread\n");
    return 2;
  }

  if(pthread_join(req_2, NULL)) {
    fprintf(stderr, "Error joining thread\n");
    return 2;
  }

  if(pthread_join(req_3, NULL)) {
    fprintf(stderr, "Error joining thread\n");
    return 2;
  }

  if(pthread_join(req_4, NULL)) {
    fprintf(stderr, "Error joining thread\n");
    return 2;
  }
  fclose(blacklist);
  close(sd);
  return 0;
}


