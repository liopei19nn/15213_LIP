#include "csapp.h"
#include <pthread.h>

#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

typedef struct node{
	char *uri;
	char *data;
	struct node* prev_node;
	struct node* next_node;
	unsigned int size;
}node;

typedef struct cache{
	node* head;
	unsigned int size;
}cache;

void init_cache(cache* my_cache);
node* find_node(cache* my_cache, char* in_uri);
void add_cache(cache* my_cache, char* inuri, char* indata, unsigned int data_size);
void remove_last_node (cache* my_cache);
void move_node_to_head(cache* my_cache, node* current_node);
void print_cache(cache* my_cache);
