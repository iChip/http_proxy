// CODE ADAPTED FROM http://beej.us/guide/bgnet/output/print/bgnet_USLetter.pdf

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

void fillHeader(char header[], int sockfd){
  size_t buf_idx = 0;
  while (buf_idx < MAXBUF && 1 == read(sockfd, &header[buf_idx], 1))
  {
    if (buf_idx > 0          &&
        '\n' == header[buf_idx] &&
        '\r' == header[buf_idx - 1] &&
        '\n' == header[buf_idx - 2] &&
        '\r' == header[buf_idx - 3])
    {
        break;
    }
  buf_idx++;
  }
}

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

int getConLen(char headers[]){
  char * len;
  len = strstr(headers, "Content-Length: ");
  char lenArray[10];
  memset(lenArray, 0, 10);
  len = len+16;
  return (int)strtol(len, NULL, 10);
}

int checkChunked(char headers[]){
    if (strstr(headers, "Transfer-Encoding: chunked") != NULL){
      return 1;
    }
    else {
      return 0;
    }
}

int getHostContent(int sockfd, char *cb, int CBUFLEN){


//TODO VERIFY LEN == BYTES_SENT
// TODO CHUNKED GET www.ietf.org/rfc/rfc2396.txt

// FILL/PROCESS HEADER
char header[MAXBUF] = { 0 };
fillHeader(header, sockfd);
int chunkLen = getChunkLen(sockfd);

int chunkedFlag = checkChunked(header);
int contentLength;
if (chunkedFlag == 0){
  contentLength = getConLen(header);
}

//add header to message
char * fullMessage = malloc(strlen(header));
strcat(fullMessage, header);


// HANDLE CHUNK DATA
if (chunkedFlag == 1){
  int chunkSize = chunkLen;
  while (chunkSize != 0){
    fullMessage = (char *) realloc(fullMessage, (strlen(fullMessage)+chunkSize+2));
    int received = 0;

    while (received < chunkSize-1){

      char tempBuf[chunkSize];
      memset(tempBuf, 0, chunkSize);

      int tmp_rec;
      tmp_rec = recv(sockfd, tempBuf, chunkSize-received, 0);
      received += tmp_rec;
      strcat(fullMessage, tempBuf);
    }
    char nlbuf[2];
    recv(sockfd, nlbuf, 2, 0);
    strcat(fullMessage, nlbuf);
    chunkSize = getChunkLen(sockfd);
  }
}
else {
  // HANDLE CL DATA
  fullMessage = (char *) realloc(fullMessage, (strlen(fullMessage)+contentLength));
  int received = 0;
  int n;
  int rec;
  char tempBuf[256];
  memset(tempBuf, 0, 256);

  // char wha[contentLength];
  while ((n = recv(sockfd, tempBuf, 255, 0) > 0)){
      // printf("%s\n", tempBuf);
      strcat(fullMessage, tempBuf);
      rec += n;
      memset(tempBuf, 0, 256);
  }
}
if (strlen(fullMessage) > CBUFLEN){
  cb = (char *) realloc(cb, strlen(fullMessage));
}
strcpy(cb, fullMessage);
// free(fullMessage);
close(sockfd);



return 0; }
