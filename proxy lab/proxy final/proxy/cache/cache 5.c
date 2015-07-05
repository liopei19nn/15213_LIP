#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

#include "cache.h"
#include "csapp.h"
#include <stdio.h>
#include <stdlib.h>

pthread_rwlock_t lock;
void cache_init(){
    pthread_rwlock_init(&lock, 0);
}

web_object* checkCache(cache_LL* cache, char* path)
{
    printf("checking cache, if %s in cache \n",path);
    pthread_rwlock_rdlock(&lock);

    web_object* cursor = search_node(cache,path);

    if (cursor == NULL)
    {   
        pthread_rwlock_unlock(&lock);
        return NULL;
    }

    if (p != cache -> head)
    {
        delete_node(cache, p);
        insert_node(cache, p);
    }

    
    pthread_rwlock_unlock(&lock);
    return cursor;
}

/* Search a node from the linked list by the requested url. */
web_object* search_node(cache_LL* cache, char* path)
{
    web_object* cursor = cache->head;
    while(cursor != NULL)
    {
        if(!strcmp(cursor->path, path)) {
            return cursor;
        }
        cursor = cursor->next;
    }
    return NULL;
}

/* Insert a node to the front of the linked list. */
void insert_node(cache_LL* cache, web_object* p)
{
    if (p == NULL || cache == NULL)
        return;

    if (cache->head == NULL) // if it's an empty list
    {
        cache -> head = p;
        cache -> tail = p;
        p->next = NULL;
        p->previous = NULL;
        cache->size -= p->size;
        return;
    }

    p->next = cache->head;
    p->previous = NULL;

    cache->head->previous = p;
    cache->head = p;
    cache->size -= p->size;

    return;
}

/* Detele a node from the double linked list. */
void delete_node(cache_LL* cache, web_object* p)
{
    if (p == NULL || cache == NULL)
        return;
    

    if (p == cache->tail) // if it's at the end.
    {
        cache -> tail = p -> previous;
        cache -> tail -> next = 0;
    }
    else // if it's not at the front and not at the end.
    {
        p->previous->next = p->next;
        p->next->previous = p->previous;
    }

    cache->size -= p->size;

    if (p == NULL)
        return;

    if (p->path != NULL)
        Free(p->path);
    if (p->data != NULL)
        Free(p->data);
    Free(p);
    return;
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

