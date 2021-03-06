//ID:lip
//Name:Li Pei
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include "cachelab.h" //to get printSummary function
//
int num_miss = 0,num_hit = 0,num_eviction = 0;
//s = num of set, E = num of line, b = num of space
int s = 0,E = 0,b = 0,set_size=0;
//
int original_age = 0;
//cache_set_line
//valid = 1, line_tag = tag, age_of_line = E -> 1, or 0 for valid = 0
typedef struct{
	int valid;
	int line_tag;
	int age_of_line;
}Cache_Line;

//cache_set
//*line_array = first_line_index
typedef struct{
	Cache_Line *line_array;
}Cache_Set;
//cache
//*set_array = first_line_set
typedef struct{
	Cache_Set *set_array;
}Cache;



char *read_in_file = NULL;
//get L,M,N get s,b
void get_operation(int argc, char **argv);
int get_set(int address, int s, int b);
int get_tag(int address, int s, int b);
void read_file(Cache *cache);

//
void intialize_cache(Cache *cache);
void operate_cache(Cache *cache,char identifier,int address,int size);
int is_cache_full(Cache *cache,int set);

//load for L, store for S, modify for M 

void operate_load(Cache *cache, int set, int tag);
void operate_store(Cache *cache,int set,int tag);
void operate_modify(Cache *cache,int set,int tag);

//LRU for age
void refresh_age(Cache *cache, int set, int line);

//free malloc
void free_cache(Cache *cache);


// Set new-visit line's age to E
// for other line, if line age > new-visit  line's last-age, and line is valid
// line age-- 
void refresh_age(Cache *cache, int set, int line)
{
    int last_age = cache -> set_array[set].line_array[line].age_of_line;
    cache -> set_array[set].line_array[line].age_of_line  = E;
    for(int line_num = 0; line_num < E; line_num++)
    {
        if(line_num == line)
        {
            continue;
        }
        
        int current_age = cache -> set_array[set].line_array[line_num].age_of_line;
        
        if ((current_age > last_age) && (cache -> set_array[set].line_array[line_num].valid == 1) )
        {
            cache -> set_array[set].line_array[line_num].age_of_line --  ;
        }   
    }    
}

void get_operation(int argc, char **argv)
{
	int opt;
    int opt_count;
	//int opt_count = 0;
	while(-1 != (opt = getopt(argc, argv, "s:E:b:t:")))
	{
		switch(opt){
			case 's':
				s = atoi(optarg);
				set_size = 1 << s;
				opt_count++;
				break;
			case 'E':
				E = atoi(optarg);
				opt_count++;
				break;
			case 'b' :
				b = atoi(optarg);
				opt_count++;
				break;
			case 't' :
				read_in_file = optarg;
				opt_count++;
				break;
			default:
                printf("Illegal operation argument.\n");
                exit(0);
		}
	}
	
	if(opt_count != 4)
	{
		printf("Illegal number of input operation.\n");
		exit(0);
	}
}



void read_file(Cache *cache){
	FILE *pFile;
	pFile = fopen(read_in_file,"r");
	char identifier;
	int address;
	int size;
	// Reading lines like " M 20,1" or "L 19,3"
	while(fscanf(pFile," %c %x,%d",&identifier,&address,&size) > 0){
		if(identifier == 'I'){
			//Ignore instruction I
		}
		else
		{
			//Operation according to identifier
			operate_cache(cache,identifier,address,size);
		}
	}
    free_cache(cache);
	fclose(pFile);
}


// do cache operation in this funciton
void operate_cache(Cache *cache,char identifier,int address,int size)
{
	int set = get_set(address,s,b);
	int tag = get_tag(address,s,b);
	//load store or modify
	switch (identifier){
		case 'L':
			operate_load(cache,set,tag);
			break;
			
		case 'S':
			operate_store(cache,set,tag);
			break;
			
		case 'M' :
			operate_modify(cache,set,tag);
			break;
			
		default:
            printf("Illegal operation argument.\n");
            exit(0);
	}
}

void operate_load(Cache *cache, int set, int tag)
{
	int hit_flag = 0;
	for(int line_num = 0; line_num < E; line_num ++){
		Cache_Line current_line = cache -> set_array[set].line_array[line_num];
		if ((current_line.valid == 1) && (current_line.line_tag == tag))
		{
			hit_flag = 1;
			num_hit ++;
			refresh_age(cache, set, line_num);
			break;
		}
	}
	//when there is no hit in cache
	if(hit_flag != 1){
		num_miss ++;
		//when the cache is not full
		if(is_cache_full(cache,set) != 1)
		{
			for(int line_num = 0; line_num < E; line_num ++)
			{
				if(cache -> set_array[set].line_array[line_num].valid == 0){
					cache -> set_array[set].line_array[line_num].line_tag = tag;
					cache -> set_array[set].line_array[line_num].valid = 1;
					refresh_age(cache, set, line_num);
					break;
				}
			}
		}
		//when the cache is full
		else{
			num_eviction ++;
            int minimum_age = cache -> set_array[set].line_array[0].age_of_line;
            int minimum_line = 0;
            for (int line_num = 1; line_num < E; line_num++ ) {
                if ((cache -> set_array[set].line_array[line_num].age_of_line) < minimum_age ) {
                    minimum_age = cache -> set_array[set].line_array[line_num].age_of_line;
                    minimum_line = line_num;
                }
            }
            cache -> set_array[set].line_array[minimum_line].line_tag = tag;
            refresh_age(cache, set, minimum_line);
		}
	}
	
}

void operate_store(Cache *cache,int set,int tag){
	int hit_flag = 0;
	for(int line_num = 0; line_num < E; line_num ++){
		Cache_Line current_line = cache -> set_array[set].line_array[line_num];
		if ((current_line.valid == 1) && (current_line.line_tag == tag))
		{
			hit_flag = 1;
			num_hit ++;
			refresh_age(cache, set, line_num);
			break;
		}
    }
    if(hit_flag != 1){
        operate_load(cache, set, tag);
    }
}

void operate_modify(Cache *cache,int set,int tag){
	operate_load(cache,set,tag);
    operate_store(cache, set, tag);
}



int is_cache_full(Cache *cache, int set){
    int full_flag = 1;
    
    for(int line_num = 0; line_num < E; line_num++){
        if((cache -> set_array[set].line_array[line_num].valid) == 0){
            full_flag = 0;
            break;
        }
    }
    
	return full_flag;
}

int get_set(int address, int s, int b){
	return (address >> b) & ((1 << s) - 1);
}

int get_tag(int address, int s, int b){
	return (address >> (s + b)) & 0x7fffffff;
}

void initialize_cache(Cache *cache)
{
	cache -> set_array = (Cache_Set *) malloc ((set_size) * sizeof(Cache_Set));
	for (int set_num = 0; set_num < set_size; set_num++)
	{
		cache -> set_array[set_num].line_array = (Cache_Line *) malloc(E * sizeof(Cache_Line));
		for (int line_num = 0; line_num < E; line_num++){
			cache -> set_array[set_num].line_array[line_num].valid = 0;
            cache -> set_array[set_num].line_array[line_num].age_of_line = original_age;
            cache -> set_array[set_num].line_array[line_num].line_tag = 0;
		}
	}
}

void free_cache(Cache *cache){
    cache -> set_array = (Cache_Set *) malloc ((set_size) * sizeof(Cache_Set));
    
    for (int set_num = 0; set_num < set_size; set_num++)
    {
        free(cache ->set_array[set_num].line_array);
    }
    free(cache -> set_array);
}




int main(int argc, char **argv)
{
	get_operation(argc, argv);
	Cache cache;
    initialize_cache(&cache);
    read_file(&cache);
    printSummary(num_hit,num_miss,num_eviction);
	return 0;
}


