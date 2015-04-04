#include "Request.h"
#include "Cache.h"
#include "host_connection.h"

#define CBUFLEN 100000

const char* DEFAULT_PORT = "80";
const char* e403 = "403 - Forbidden";
const char* e500 = "500 - Internal Server Error";
const char* cat_expr_1 = "gsed -r -i 's_(.*<img src=\")([^\"]*)(.*)_\\1http://thecatapi.com/api/images/get\?format=src\\";
const char* cat_expr_2 = "&type=jpg\\3_g' ";

void parse_request(Request* r, char* buf, int buf_len)
{
  // Ignore 'GET '
  char* start = buf+4;
  if (strstr(buf, "http://") != NULL){
      char* tmp = start+7;
      start = tmp;
  }
  char* end = start;
  while(1){
    if (*end=='/') break;
    end++;
  }
  memcpy(r->server_host, start, end-start);
  r->server_url[end-start] = '\0';

  start = end;
  while(1){
    if (*end==':' || *end == ' '|| *end=='\n' || *end=='\r') break;
    end++;
  }
  memcpy(r->server_url, start, end-start);
  r->server_url[end-start] = '\0';
  start = end;
  if (*end == ':'){
    start++;
    while(1){
      if (*end==' ') break;
      end++;
    }
    memcpy(r->server_port, start, end-start);
    r->server_port[end-start] = '\0';
  } else {
    strcpy(r->server_port, DEFAULT_PORT);
  }

  char* GET = "GET ";
  char* HTTPVER = " HTTP/1.1\r\n";
  char* END = "\r\n\r\n";
  char* HOST = "Host: ";
  char* COLON = ":";
  strcat(r->formatted_request, GET);
  strcat(r->formatted_request, r->server_url);
  strcat(r->formatted_request, HTTPVER);
  strcat(r->formatted_request, HOST);
  strcat(r->formatted_request, r->server_host);
  strcat(r->formatted_request, COLON);
  strcat(r->formatted_request, r->server_port);
  strcat(r->formatted_request, END);

  // Format For File Name
  strcat(r->absolute_url,r->server_host);
  strcat(r->absolute_url,r->server_url);
  strcat(r->absolute_url,COLON);
  strcat(r->absolute_url,r->server_port);

  printf("Server Host: %s\n",r->server_host);
  printf("Server Resource: %s\n",r->server_url);
  printf("Server Port: %s\n",r->server_port);
  printf("Formatted Request: \n%s\n", r->formatted_request);
  printf("Absolute Url: %s\n", r->absolute_url);
}

void* process_request(void* args)
{
  Thread_Args* thread_args = (Thread_Args*)args;
  int sd = thread_args->socket_descriptor;
  Request* r = &(thread_args->r);
  int client_len;
  struct sockaddr_in client;
  char sed_cmd[512];
  FILE* blacklist = thread_args->blacklist;


  char buf[BUFLEN];
  int client_socket;
  while(1)
  {
    if ((client_socket = accept(sd, (struct sockaddr *)&client, (socklen_t*)&client_len)) == -1) {
      fprintf(stderr, "Can't accept client.\n");
      exit(1);
    }

    /* Client Accepted */
    memset(buf, 0, BUFLEN);
    char* get = "GET";
    char* bp;
    bp = buf;
    int bytes_to_read, n;
    bytes_to_read = BUFLEN;
    while((n = read(client_socket, bp, bytes_to_read)) > 0) {
      if (*bp == '\n') {
        break;
      }
      bp += n;
      bytes_to_read -= n;
    }

    /* If Not Get Request Return 404 */
    int valid;
    char* invalid = "403 - Only allowed method is GET.\r\n\r\n";
    valid = strncmp(buf, get, 3);
    if ((valid = strncmp(buf, get, 3)) != 0){
      write(client_socket,invalid,strlen(invalid));
      close(client_socket);
      continue;
    }

    /* Parse Client Request */
    reset_request(r);
    parse_request(r, buf, BUFLEN);

    /* Validate Against Blacklist */
    int cleared = check_blacklist(r->server_host, blacklist);

    if (!cleared){
      write(client_socket, e403, strlen(e403));
      close(client_socket);
      continue;
    }

    /* Check Cache Before Proceeding */
    char cache_file_path[250];
    memset(cache_file_path,0,250);
    get_file_path(r->absolute_url, cache_file_path);
    FILE* cache_file;
    int cache_hit = file_exists(cache_file_path);

    if (cache_hit)
    {
      cache_file = fopen(cache_file_path, "r");
      if (!cache_file) {
        perror("cache file error");
        continue;
      }
      printf("cache hit!");

      // Read File and Transmit Over Client Socket
      read_cache(cache_file, client_socket);
      close(client_socket);
    }
    else
    {
      /* Cache Miss */

      /* Initialize cache_file */
      char* temp_suffix = "_temp";
      char temp_file_path[BUFLEN];
      strcpy(temp_file_path, cache_file_path);
      strcat(temp_file_path,temp_suffix);

      /* Use Temporary Filename */
      cache_file = fopen(temp_file_path,"w+");
      if (!cache_file) perror("Error");

      /* Create a stream socket. */
      int sockfd, numbytes;
      char server_buf[BUFLEN];
      struct addrinfo hints, *servinfo, *p;
      int rv;
      char s[INET6_ADDRSTRLEN];

      memset(&hints, 0, sizeof hints);
      hints.ai_family = AF_UNSPEC;
      hints.ai_socktype = SOCK_STREAM;

      /* Get Address Information */
      if ((rv = getaddrinfo(r->server_host, r->server_port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        freeaddrinfo(servinfo);
        return NULL;
      }

      /* Open First New Socket to Requested HTTP Server */
      for (p = servinfo; p != NULL; p = p->ai_next)
      {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
          perror("client: socket");
          continue;
        }
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
          close(sockfd);
          perror("client: connect");
          continue;
        }
        break;
      }

      if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        freeaddrinfo(servinfo);
        write(client_socket, e500, strlen(e500));
        close(client_socket);
        return NULL;
      }

      inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
      printf("client: connecting to %s\n", s);
      freeaddrinfo(servinfo);

      /* Write Original Client Request to Socket, Update Cache */
      write(sockfd, r->formatted_request, strlen(r->formatted_request));
      printf("Sent: %s\n", r->formatted_request);

      /* Get Host Content From Server Response */
      char clientBuffer[CBUFLEN];
      memset(clientBuffer, 0, CBUFLEN);
      getHostContent(sockfd, clientBuffer, CBUFLEN);

      /* Write Retrieved Content to Cache File */
      int bytes_cached = fwrite(clientBuffer, 1, strlen(clientBuffer), cache_file);

      //  Write to socket and send to the client.
      int to_send = strlen(clientBuffer);

      /* Catify */
      memset(sed_cmd, 0, 512);
      strcpy(sed_cmd, cat_expr_1);
      strcat(sed_cmd, cat_expr_2);
      strcat(sed_cmd, temp_file_path);
      printf("Sed: %s\n",sed_cmd);
      system(sed_cmd);

      sleep(1);

      read_cache(cache_file, client_socket);

      // /* Rename Cache File */
       int renamed = rename(temp_file_path, cache_file_path);
       if (renamed == -1) perror ("Error");

      /* Clean up. */
      close(client_socket);
    }
  }
}

/* Return True if host is Blacklisted */
int check_blacklist(char* host, FILE* blacklist){
  int MAX_LENGTH = 101;
  char *found, line[MAX_LENGTH];
  while( fgets (line, MAX_LENGTH, blacklist) != NULL ) {
    // Remove Trailing Crap From fgets
    line[strcspn(line, "\r\n")] = 0;
    found = strstr(host, line);
    if (found){
      printf("Request for host %s is blacklisted under entry %s\n", host, line);
      return 0;
    }
  }
  return 1;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
 if (sa->sa_family == AF_INET) {
 return &(((struct sockaddr_in*)sa)->sin_addr);
 }
 return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/* Reset Request Information */
void reset_request(Request* request)
{
  memset(request->absolute_url, 0, BUFLEN);
  memset(request->server_host, 0, BUFLEN);
  memset(request->server_url, 0, BUFLEN);
  memset(request->server_port, 0, BUFLEN);
  memset(request->formatted_request, 0, BUFLEN);
}


