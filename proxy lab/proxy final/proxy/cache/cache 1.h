#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <getopt.h>
#include <stdlib.h>

typedef struct web_object{
  char *data;
  unsigned int size;
  char* path;
  struct web_object* next;
  struct web_object* previous;
} web_object;

typedef struct cache_LL{
  web_object* head;
  web_object* tail;
  unsigned int size;
}cache_LL;

void cache_init();
web_object* checkCache(cache_LL* cache, char* path);
void addToCache(cache_LL* cache, char* data, char* path, unsigned int addSize);
void evictAnObject(cache_LL* cache);
