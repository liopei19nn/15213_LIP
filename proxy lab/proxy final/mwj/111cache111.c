/*
 * simple cache using the least recent used eviction 
 * strategy. This cache is implemented by a doublely
 * linked list, a new block is moved to the head of 
 * the linked list. And evictions only happens at the
 * tail of the linked list.
 *
 * Name:        Wenjie Ma
 * AndrewID:    wenjiem
 */

#include "csapp.h"
#include "cache.h"
#include <pthread.h>

#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

pthread_rwlock_t lock;

/*
 * init_cache - initialize the cache
 */
void init_cache(cache* my_cache) 
{
    pthread_rwlock_init(&lock, 0);
}

/*
 * find_node - given the uri, find data in the cache
 *          if found return the node, else return null
 */
node* find_node(cache* my_cache, char* in_uri) 
{
    //printf("CACHE >> inside find_node\n");
    pthread_rwlock_rdlock(&lock);
    //printf("CACHE >> inside find_node mutex");
    //printf("\nCACHE >> checking cache for %s\n", in_uri);
    if (my_cache->head == NULL){
        //printf("\nCACHE >> cache is empty!\n");
        pthread_rwlock_unlock(&lock);
        return NULL;
    }

    node* current_node;
    for (current_node = my_cache->head; 
        current_node->next_node != NULL;
        current_node = current_node->next_node) {
        //printf("CACHE >> Compare to %s\n", current_node->uri);

        if (!strcmp(current_node->uri, in_uri)) {
            pthread_rwlock_unlock(&lock);
            pthread_rwlock_wrlock(&lock);
            move_node_to_head(my_cache, current_node);
            printf("CACHE >> found in cache!\n");
            pthread_rwlock_unlock(&lock);
            return current_node;
        }
    }

    printf("CACHE >> Not found in cache!\n");
    pthread_rwlock_unlock(&lock);
    return NULL;
}

/*
 * move_node_to_head - move a recent used node to the head position
 *                  of the double linked list
 */
void move_node_to_head(cache* my_cache, node* current_node) 
{
    node* tmp_node;
    // case 1 node is at head position
    if (current_node == my_cache->head) {
        return;
    }

    // case 2 if tail node
    if (current_node->next_node == NULL) {
        // update prev node
        tmp_node = current_node->prev_node;
        tmp_node->next_node = current_node->next_node;

        //update head
        current_node->prev_node = NULL;
        current_node->next_node = my_cache->head;
        my_cache->head->prev_node = current_node;
        my_cache->head = current_node;

    // case 3 if other node
    } else {
        // update prev node
        tmp_node = current_node->prev_node;
        tmp_node->next_node = current_node->next_node;

        //update next node
        tmp_node = current_node->next_node;
        tmp_node->prev_node = current_node->prev_node;

        //update head
        current_node->prev_node = NULL;
        current_node->next_node = my_cache->head;
        my_cache->head->prev_node = current_node;
        my_cache->head = current_node;
    }
    return;
}

/*
 * add_cache - add data to the cache, in another word, to the 
 *          head position of the linked list
 */
void add_cache(cache* my_cache, char* inuri, char* indata, 
    unsigned int data_size) 
{
    //printf("CACHE >> inside add_cache!\n");
    pthread_rwlock_wrlock(&lock);
    //printf("CACHE >> inside mutex!\n");
    //print_cache(my_cache);
    // create new node
    node* new_node = Malloc(sizeof(node));
    new_node->uri = Malloc(MAXLINE);
    new_node->data = Malloc(MAX_OBJECT_SIZE);

    memcpy(new_node->data, indata, data_size);
    strcpy(new_node->uri, inuri);
    new_node->size = data_size;

    //printf("CACHE >> data size is: %d\n", data_size);
    // update cache
    my_cache->size += data_size;

    // if cache is empty
    if (my_cache->head == NULL) {
        new_node->next_node = NULL;
        new_node->prev_node = NULL;
        my_cache->head = new_node;
        pthread_rwlock_unlock(&lock);
        return;
    }

    // if head is not null
    new_node->next_node = my_cache->head;
    new_node->prev_node = NULL;
    my_cache->head->prev_node = new_node;

    my_cache->head = new_node;

    //printf("CACHE >> cache size is: %d, 
    //max cahce size is: %d\n", my_cache->size, MAX_CACHE_SIZE);

    while(my_cache->size > MAX_CACHE_SIZE) {
        remove_last_node(my_cache);
        //printf("CACHE >> in the loop cache size is: %d", my_cache->size);
    }
    //print_cache(my_cache);

    pthread_rwlock_unlock(&lock);
    return;
}

/*
 * remove_last_node - remove the last node form the linked list
 *                  which is served as the evict policy
 */
void remove_last_node (cache* my_cache) {
    node* iterator;
    node* temp_node;

    // locate the last node
    iterator = my_cache->head;
    while(iterator->next_node != NULL)
        iterator = iterator->next_node;

    // update double linked list
    my_cache->size -= iterator->size;
    temp_node = iterator->prev_node;
    temp_node->next_node = NULL;

    // always free memory
    free(iterator->uri);
    free(iterator->data);
    free(iterator);
    return;
}

/*
 * print_cache - print all the cache by iterating
 *          through the linked list
 */
void print_cache(cache* my_cache) 
{
    pthread_rwlock_rdlock(&lock);

    if (my_cache->head == NULL) {
        printf("\n\nCACHE >> Cache is empty!!\n\n");
        pthread_rwlock_unlock(&lock);
        return;
    }

    node* current_node;
    for (current_node = my_cache->head; 
        current_node->next_node != NULL;
        current_node = current_node->next_node) {
        printf("\ncache includes uri is: %s\n",current_node->uri);
    }

    pthread_rwlock_unlock(&lock);
    return;
}














