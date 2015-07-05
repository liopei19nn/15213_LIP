/*
 * This file is the implement of cacheuse.h functions, for 15-213 proxylab.
 *
 * The basic idea for the implement of the data structure of cache is the
 * double linked list (DLL). The double linked list is used as the priority
 * queue, with the least accessed node at the head of the list, and the most
 * recent accessed node at the end of the list.
 *
 * cacheuse.h and cacheuse.c files are used as the interface of functions to
 * handle the cache from the proxy.c.
 */

/* Header Files */
#include "csapp.h"
#include "cache.h"
#include "cacheuse.h"

/******* Detailed Implement of Functions *******/


/* Find a node in the linked list based on the url. */
node* find_node(cache* list, char* url)
{
    if (list == NULL || url == NULL)
        return NULL;

    //reader preference when searching nodes
    P (&(list->mutex_r));
    list->readcnt++;
    if (1 == list->readcnt)
        P (&(list->mutex_w));
    V (&(list->mutex_r));

    node* p = search_node(list, url);
    
    P (&(list->mutex_r));
    readcnt--;
    if (0 == list->readcnt)
        V (&(list->mutex_w));
    V (&(list->mutex_r));


    if (p == NULL)
        return NULL;


    P(&list->mutex_w);
    delete_node(list, p);
    insert_node_end(list, p);
    V(&list->mutex_w);

    return p;
}

/* Add a node to the linked list with the current data from the server. */
void add_node(cache* list, char* url, char* data, int length)
{
    node* p;
    if (list == NULL)
        return;

    p = new_node(url, data, length);
    if (p == NULL)
        return;

    if (list->empty_size >= p->size)
    {
        P(&list->mutex_w);
        insert_node_end(list, p);
        V(&list->mutex_w);
    }
    else
    {
        P(&list->mutex_w);
        evict_node(list, p);
        insert_node_end(list, p);
        V(&list->mutex_w);
    }

    return;
}

/* If the space is not enough, do eviction.
 * Delete the least accessed node to give out more space. */
void evict_node(cache* list, node* p)
{
    if (list == NULL || p == NULL)
        return;

    node* bp = list->front;
    P(&list->mutex_w);
    while (list->empty_size < p->size)
    {
        delete_node(list, bp);
        free_node(bp);
        bp = list->front;
    }
    V(&list->mutex_w);

    return;
}