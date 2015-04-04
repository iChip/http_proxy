#ifndef _CACHE_317_H
#define _CACHE_317_H

#include <stdio.h>
#include <stdlib.h>
#include "SuperFastHash.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

struct Cache {
  FILE* cache_dir;
};

/* Initialize Cache Directory on Server Start */
void init_cache_dir();

/* Check For Existence of File file_name */
int file_exists(char* file_name );

/* Read Contents From file, Write to Socket Descriptor sd */
void read_cache(FILE* file, int sd);

/* Create Valid File Name From url */
void generate_filename(char* url, char* dest);

/* Utility Fn to Send All Bytes From File */
int sendall(int s, char *buf, int *len);

/* Write Buffer To Cache From Server Response */
void write_cache(char* url, char* buffer, int len);

/* Get File Path */
void get_file_path(char* url, char* dest);

/* Check if Cache Directory is Writable */
int is_dir_writable(char* str);


#endif