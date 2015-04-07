#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define BUFLEN 256
#define MAXBUF 1024


/* Store contents of header */
void fillHeader(char header[], int sockfd){
  size_t buf_i = 0;
  while (buf_i < MAXBUF && 1 == read(sockfd, &header[buf_i], 1))
  {
    if (buf_i > 0          &&
        '\n' == header[buf_i] &&
        '\r' == header[buf_i - 1] &&
        '\n' == header[buf_i - 2] &&
        '\r' == header[buf_i - 3])
    {
        break;
    }
  buf_i++;
  }
}

/* Get the length of the next chunk (chunked encoding) */
int getChunkLen(int sockfd){
  char rawLen[100];
  memset(rawLen, 0, 100);
  size_t buf_idx = 0;
  while (buf_idx < MAXBUF && 1 == read(sockfd, &rawLen[buf_idx], 1))
  {
    if (buf_idx > 0          &&
        '\n' == rawLen[buf_idx])
    {
        break;
    }
  buf_idx++;
  }
  return (int)strtol(rawLen, NULL, 16);
}

int checkResponse(char headers[]){
  char * resp;
  char * respEdit;
  int code;
  resp = strstr(headers, " ");
  resp = resp+1;
  strncpy(respEdit, resp, 3);
  printf("HTTP Server Response Code: %s\n", respEdit);
  code = (int)strtol(respEdit, NULL, 10);
  if (code != 200){
    return 0;
  }
  else {
    return 1;
  }
}

/* Get length of the message (non-chunked encoding) */
int getConLen(char headers[]){
  char * len;
  len = strstr(headers, "Content-Length: ");
  char lenArray[10];
  memset(lenArray, 0, 10);
  len = len+16;
  return (int)strtol(len, NULL, 10);
}

/* Check if encoding is chunked */
int checkChunked(char headers[]){
    if (strstr(headers, "Transfer-Encoding: chunked") != NULL){
      return 1;
    }
    else {
      return 0;
    }
}

// Processes response from server
int getHostContent(int sockfd, char *cb, int CBUFLEN){

// Process header
char header[MAXBUF] = { 0 };
fillHeader(header, sockfd);

int respValid;
respValid = checkResponse(header);

int chunkLen = 0;
int chunkedFlag = checkChunked(header);
int contentLength;
if (chunkedFlag == 0){
  contentLength = getConLen(header);
}
else {
  int chunkLen = getChunkLen(sockfd);
}

// Add header to message
char * fullMessage = calloc(1, strlen(header));
strcat(fullMessage, header);

// HANDLE CHUNK DATA
if (chunkedFlag == 1){
  int chunkSize = chunkLen;
  while (chunkSize != 0){
  // realloc handle code adapted from:
  // http://stackoverflow.com/questions/27589846/dynamic-arrays-using-realloc-without-memory-leaks
    void * tmp = (char *) realloc(fullMessage, (strlen(fullMessage)+chunkSize));
    if (NULL == tmp)
    {
      printf("%s\n", "Out of memory.");
    }
    else
    {
      fullMessage = tmp;
    }

    int received = 0;

    while (received < chunkSize-1){

      char tempBuf[chunkSize];
      memset(tempBuf, 0, chunkSize);

      int tmp_rec = 0;
      tmp_rec = recv(sockfd, tempBuf, chunkSize-received, 0);
      received += tmp_rec;
      strcat(fullMessage, tempBuf);
    }
    char nlbuf[2];
    recv(sockfd, nlbuf, 2, 0);
    chunkSize = getChunkLen(sockfd);
  }
}
else {
  // HANDLE NON-CHUNKED DATA
  // realloc handle code adapted from:
  // http://stackoverflow.com/questions/27589846/dynamic-arrays-using-realloc-without-memory-leaks
  void * tmp = (char *) realloc(fullMessage, (strlen(fullMessage)+contentLength));
  if (NULL == tmp)
  {
    printf("%s\n", "Out of memory.");
  }
  else
  {
    fullMessage = tmp;
  }

  int received = 0;
  int n;
  int rec = 0;
  char tempBuf[256];
  memset(tempBuf, 0, 256);

  while ((n = recv(sockfd, tempBuf, 255, 0) > 0) || strlen(fullMessage)-strlen(header) < contentLength){
      strcat(fullMessage, tempBuf);
      rec += n;
      memset(tempBuf, 0, 256);
  }
}

if (strlen(fullMessage) > CBUFLEN){
  cb = (char *) realloc(cb, strlen(fullMessage));
}
  strcpy(cb, fullMessage);
  free(fullMessage);
  close(sockfd);

  return respValid;
}
