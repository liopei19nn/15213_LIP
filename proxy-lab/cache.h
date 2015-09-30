/*
* Proxy's cache
* Name: Li Pei
* Andrew ID: lip
*/

#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

#include <stdio.h>
#include <stdlib.h>

typedef struct web_object{
  char *data;       //save data
  unsigned int size;
  char* path;       //save URL
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
void moveToFront(cache_LL* cache, web_object* p);
web_object* search_node(cache_LL* cache, char* path);