/*
* Proxy's cache
* Name: Li Pei
* Andrew ID: lip
* 
* cache function
* save the recently web objects in cache
* using doubly linked list with LRU strategy,
* i.e. add and visit are all considered as
* recent use
*/

#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#include "cache.h"
#include "csapp.h"
#include <stdio.h>
#include <stdlib.h>
// reader's lock
pthread_rwlock_t read_lock;
// writer's lock
pthread_rwlock_t write_lock;

//Initialize cache with read and write lock
void cache_init(){
    pthread_rwlock_init(&read_lock, NULL);
    pthread_rwlock_init(&write_lock, NULL);
}
// Check if the content corresponding to URL
// is in the cache, and return this object
// according to LRU, put it in the head of 
// cache
web_object* checkCache(cache_LL* cache, char* path)
{    
    // search the URL in cache   
    web_object* cursor = search_node(cache,path);

    if (cursor == NULL)
    {
        return cursor;
    }
    // if find it at head, return
    // if not, put it in head
    if (cursor != cache -> head)
    {   
        // move the recently used web object
        // in the front
        moveToFront(cache, cursor);
    }
    return cursor;
}

// search the recently used web object in cache
web_object* search_node(cache_LL* cache, char* path)
{
    // add reader's lock, block other reader and writer
    pthread_rwlock_rdlock(&read_lock);
    web_object* cursor = cache->head;
    while(cursor != NULL)
    {
        if(!strcmp(cursor->path, path)) {
             // unlock reader's lock
            pthread_rwlock_unlock(&read_lock);
            return cursor;
        }
        cursor = cursor->next;
    }
    // unlock reader's lock
    pthread_rwlock_unlock(&read_lock);
    return NULL;
}

// move the recently used web object to the head
// of cache
void moveToFront(cache_LL* cache, web_object* p)
{
    //add writers lock to block other writers
    pthread_rwlock_wrlock(&write_lock);

    // if it is at tail
    if (p == cache->tail)
    {
        cache -> tail = p -> previous;
        cache -> tail -> next = NULL;

    }
    else // if it's not at tail
    {
        p->previous->next = p->next;
        p->next->previous = p->previous;

    }

    p->next = cache->head;
    p->previous = NULL;
    cache->head->previous = p;
    cache->head = p;


    //unlock writer's lock
    pthread_rwlock_unlock(&write_lock);
    return;
}

// add a new web object in the cache
void addToCache(cache_LL* cache, char* data, char* path, unsigned int addSize)
{
    web_object* toAdd = Calloc(1, sizeof(web_object));
    toAdd->data = Calloc(1, MAX_OBJECT_SIZE);
    memcpy(toAdd->data, data, addSize);
    toAdd->path = Calloc(1, MAXLINE);
    strcpy(toAdd->path, path);
    //update the size of the new object
    toAdd->size = addSize;

    //add writers lock to block other writers
    pthread_rwlock_wrlock(&write_lock); 
    //Increment the cache size
    cache->size += addSize;


    // if there is not enough space for this object
    // evict object at tail
    while(cache->size > MAX_CACHE_SIZE)
    {
        evictAnObject(cache);
    }
    // if the cache is empty
    if (cache -> head == NULL)
    {
        toAdd -> previous = NULL;
        toAdd -> next = NULL;
        cache -> head = toAdd;
        cache -> tail = toAdd;
    }
    else    //if the cache is not empty
    {
        toAdd -> previous = NULL;
        toAdd->next = cache->head;
        cache->head->previous = toAdd;
        cache->head = toAdd;
    }
    //unlock writer's lock
    pthread_rwlock_unlock(&write_lock);
}

// evict an object from tail
void evictAnObject (cache_LL* cache)
{   
    //add writers lock to block other writers
    pthread_rwlock_wrlock(&write_lock); 

    web_object* cursor = cache->tail;

    // if there is only one object in cache
    if (cursor == cache -> head)
    {
        pthread_rwlock_unlock(&write_lock);
        return;
        
    }else{
        cache -> tail = cursor -> previous;
        cache -> tail -> next = NULL;
        cache -> size -= cursor -> size;
        Free(cursor -> data);
        Free(cursor -> path);
        Free(cursor);
    }
    //unlock writer's lock
    pthread_rwlock_unlock(&write_lock);
}