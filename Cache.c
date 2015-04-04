#include "Cache.h"
#define MAX_NAME_CHARS 254

const char* CACHE_DIR = "cache/";

/* Initialize Cache Directory on Server Start */
void init_cache_dir()
{
  struct stat st = {0};

  if (stat(CACHE_DIR, &st) == -1) {
    mkdir(CACHE_DIR, 0700);
  }
  int writable = is_dir_writable("cache");
  if (!writable)
  {
      printf("Cache directory is not writable.");
      exit(1);
  }
}

/* Read Contents From file, Write to Socket Descriptor sd */
void read_cache(FILE* file, int sd)
{
  char* buffer;
  int lSize;

  // obtain file size:
  fseek (file , 0 , SEEK_END);
  lSize = ftell (file);
  rewind (file);

  // allocate memory to contain the whole file:
  buffer = (char*) malloc (sizeof(char)*lSize);
  if (buffer == NULL) {perror("Memory error"); exit (2);}

  // copy the file into the buffer:
  int result = fread (buffer,1,lSize,file);
  if (result != lSize) {perror ("Reading error"); exit (3);}

  /* the whole file is now loaded in the memory buffer. */
  if (sendall(sd, buffer, &lSize) == -1) {
    perror("sendall");
    printf("We only sent %d bytes because of the error!\n", lSize);
  }

  // Clean Up
  fclose (file);
  free (buffer);
}

/* Write Buffer To Cache From Server Response */
void write_cache(char* url, char* buffer, int len)
{
  char hashed_filename[33];
  generate_filename(url, hashed_filename);
  char file_path[MAX_NAME_CHARS];
  strcpy(file_path, CACHE_DIR);
  strcat(file_path, hashed_filename);

  FILE* file = fopen(file_path, "w");

  fwrite(buffer, 1 , len , file );
}

/* Return Whether Wrote All Bytes To Socket */
int sendall(int s, char *buf, int *len)
{
 int total = 0; // how many bytes we've sent
 int bytesleft = *len; // how many we have left to send
 int n;
 while(total < *len) {
   n = send(s, buf+total, bytesleft, 0);
   if (n == -1) { break; }
   total += n;
   bytesleft -= n;
 }
 *len = total; // return number actually sent here
 return n==-1?-1:0; // return -1 on failure, 0 on success
}

/* Create Valid File Name From url */
void generate_filename(char* url, char* dest)
{
  uint32_t digest;
  digest = SuperFastHash(url, strlen(url));

  char temp[MAX_NAME_CHARS];
  memset(temp,0,MAX_NAME_CHARS);
  sprintf(temp, "%x",digest);
  printf("Attempted hash string: %s\n", temp);
  strcpy(dest, temp);
}

void get_file_path(char* url, char* dest)
{
  char hashed_filename[MAX_NAME_CHARS];
  generate_filename(url, hashed_filename);
  printf("Generated hash %s from url %s\n", hashed_filename, url);
  char file_path[MAX_NAME_CHARS];
  memset(file_path,0,MAX_NAME_CHARS);
  strcpy(file_path, CACHE_DIR);
  strcat(file_path, hashed_filename);
  printf("Generated file path from hash: %s\n",file_path);
  strcpy(dest, file_path);
}

/* Check For Existence of File file_name */
int file_exists (char *file_name)
{
  struct stat st = {0};
  return (stat (file_name, &st) == 0);
}

int is_dir_writable(char* str)
{
    if(access(str, W_OK) == 0) {
        return 1;
    } else {
        return 0;
    }
}