#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

#include "cache.h"
#include "csapp.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <getopt.h>
#include <stdlib.h>

pthread_rwlock_t lock;
void cache_init(){
	pthread_rwlock_init(&lock, 0);
}

web_object* checkCache(cache_LL* cache, char* path)
{
	printf("checking cache, if %s in cache \n",path);
	pthread_rwlock_rdlock(&lock);
	web_object* cursor = cache->head;
	while(cursor != NULL)
    {
        if(!strcmp(cursor->path, path)) {
            return cursor;
        }
        cursor = cursor->next;
    }
    pthread_rwlock_unlock(&lock);
    return NULL;
}

void addToCache(cache_LL* cache, char* data, char* path, unsigned int addSize)
{
	pthread_rwlock_wrlock(&lock); 

	web_object* toAdd = Calloc(1, sizeof(web_object));

	toAdd->data = Calloc(1, MAX_OBJECT_SIZE);
    //We use memcpy because we have to treat data as a byte array, not a string
    memcpy(toAdd->data, data, addSize);
    //toAdd->timestamp = timecounter;
    toAdd->path = Calloc(1, MAXLINE);
    strcpy(toAdd->path, path);
    //update the size of the new object
    toAdd->size = addSize;
    //Increment the cache size
    cache->size += addSize;


    while(cache->size > MAX_CACHE_SIZE)
    {
        evictAnObject(cache);
    }

    if (cache -> head == NULL)
    {
    	toAdd -> previous = NULL;
    	toAdd -> next = NULL;
    	cache -> head = toAdd;
    	cache -> tail = toAdd;
    }else{
    	toAdd -> previous = NULL;
    	toAdd->next = cache->head;
    	cache->head->previous = toAdd;
    	cache->head = toAdd;
    }




    pthread_rwlock_unlock(&lock);



}


void evictAnObject (cache_LL* cache)
{
	pthread_rwlock_wrlock(&lock);
	web_object* cursor = cache->tail;

	cache -> size -= cursor -> size;

	cache -> tail = cursor -> previous;

	cache -> tail -> next = NULL;

	Free(cursor -> data);
	Free(cursor -> path);
	
	Free(cursor);
	pthread_rwlock_unlock(&lock);

}








