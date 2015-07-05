/*
 * This file is the implement of cache.h functions, for 15-213 proxylab.
 *
 * The basic idea for the implement of the data structure of cache is the
 * double linked list (DLL). The double linked list is used as the priority
 * queue, with the least accessed node at the head of the list, and the most
 * recent accessed node at the end of the list.
 *
 * cache.h and cache.c files are used as the basic structure as well as the
 * interface of basic maintainance of the data structure.
 */


/* Header Files */
#include "csapp.h"
#include "cache.h"

/******* Detailed Implement of Functions *******/

/* Initialize the cache data structure. */
cache* init_cache()
{
    cache* cache_list = (cache*) malloc(sizeof(struct cache_header));

    cache_list->front = NULL;
    cache_list->end = NULL;
    cache_list->empty_size = MAX_CACHE_SIZE;
    Sem_init(&cache_list->mutex_w, 0, 1);
    Sem_init(&cache_list->mutex_r, 0, 1);
    cache_list->readcnt = 0;
    return cache_list;
}

/* Malloc a new node, and store the corresponding data. */
node* new_node(char* url, char* data, int length)
{
    node* p;
    if (url == NULL || data == NULL)
        return NULL;
    if ((p = (node*) malloc(sizeof(struct cache_node))) == NULL)
        return NULL;

    if ((p->url = (char*) malloc(sizeof(char) * (strlen(url) + 1))) == NULL)
        return NULL;
    else
        strcpy(p->url, url);

    p->size = length;
    if ((p->data = (char*) malloc(length)) == NULL)
        return NULL;
    else
        memcpy(p->data, data, length);
    p->next = NULL;
    p->prev = NULL;

    return p;
}

/* Detele a node from the double linked list. */
void delete_node(cache* list, node* p)
{
    if (p == NULL || list == NULL)
        return;
    
    if (p == list->front) // if it's at the front.
    {
        if (p == list->end) // if it's the only node in the list.
        {
            list->front = NULL;
            list->end = NULL;
            return;
        }
        list->front = p->next;
        list->front->prev = NULL;
    } 
    else if (p == list->end) // if it's at the end.
    {
        list->end = p->prev;
        list->end->next = NULL;
    }
    else // if it's not at the front and not at the end.
    {
        p->prev->next = p->next;
        p->next->prev = p->prev;
    }
    list->empty_size += p->size;

    return;
}

/* Free the space which is malloc to the node. */
void free_node(node* p)
{
    if (p == NULL)
        return;

    if (p->url != NULL)
        Free(p->url);
    if (p->data != NULL)
        Free(p->data);
    Free(p);

    return;
}

/* Search a node from the linked list by the requested url. */
node* search_node(cache* list, char* url)
{
    node* p;
    if (url == NULL || list == NULL)
        return NULL;

    p = list->front;
    while (p != NULL)
    {
        if (strcmp(p->url, url) == 0)
            return p;
        p = p->next;
    }

    return NULL;
}

/* Insert a node to the front of the linked list. */
void insert_node_front(cache* list, node* p)
{
    if (p == NULL || list == NULL)
        return;

    if (list->front == NULL) // if it's an empty list
    {
        list->front = p;
        list->end = p;
        p->next = NULL;
        p->prev = NULL;
        list->empty_size -= p->size;
        return;
    }

    p->next = list->front;
    list->front->prev = p;
    list->front = p;
    p->prev = NULL;
    list->empty_size -= p->size;

    return;
}

/* Insert a node to the end of the linked list. */
void insert_node_end(cache* list, node* p)
{
    if (p == NULL || list == NULL)
        return;

    if (list->front == NULL) // if it's an empty list
    {
        list->front = p;
        list->end = p;
        p->next = NULL;
        p->prev = NULL;
        list->empty_size -= p->size;
        return;
    }

    p->prev = list->end;
    list->end->next = p;
    list->end = p;
    p->next = NULL;
    list->empty_size -= p->size;

    return;
}
