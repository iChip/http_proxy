#ifndef _REQUEST_317_H
#define _REQUEST_317_H

#include <sys/socket.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define BUFLEN 1024

typedef struct
{
  char absolute_url[BUFLEN];
  char server_host[BUFLEN];
  char server_url[BUFLEN];
  char server_port[BUFLEN];
  char formatted_request[BUFLEN];
} Request;

typedef struct
{
  int socket_descriptor;
  Request r;
  FILE* blacklist;
} Thread_Args;

void parse_request(Request* r, char* buf, int buf_len);

void* process_request(void* args);

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa);

int check_blacklist(char* host, FILE* blacklist);

/* Reset Request Information */
void reset_request(Request* request);

#endif