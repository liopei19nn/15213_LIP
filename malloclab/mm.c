/**********************************************
 * Malloc Lab
 * Name : Li Pei
 * Andrew ID : lip
 **********************************************/

/**********************************************
 * Implemented Dynamic Memory Allocator Function:
 
 * malloc(size): Get size, add header,footer with
 size to get asize (aligned size), search for free
 block according to the azise and alignment in
 free block lists.
 
 * free(*bp): Get a block reference pointer and free
 this block.
 
 * mm_realloc(oldptr, newsize): Get the old pointer
 and new allocating size of a block. If the new size
 is smaller than the old, we have to truncate old.
 If old size is the same as new, we return the old
 pointer. If the old size is smaller than the new,
 call a new malloc, copy old block in and free the
 old block.
 
 * calloc(size): Call malloc() and initial every byte
 to 0
 **********************************************/

/**********************************************
 * Method Specifics:
 * Segregated List.
 * Mixed first-fit and best-fit.
 **********************************************/

/**********************************************
 * Block Style:
 * Free Block
 | Header | Prev Pointer | Next Pointer | Footer |
 4Bytes      8Bytes         8Bytes      4Bytes
 * Allocated Block
 | Header | Contents | Footer |
 4Bytes              4Bytes
 * Heap Head
 | Padding | Prelogue | Epilogue|
 4Bytes      TBD       4bytes
 * Prelogue
 |Header|    List Reference         |Footer|
 4Bytes   8*(Number Of Freelist)    4Bytes
 **********************************************/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mm.h"
#include "memlib.h"

/* If you want debugging output, use the following macro.
 When you hand in, remove the #define DEBUG line. */
#ifdef DEBUG
# define dbg_printf(...) printf(__VA_ARGS__)
# define dbg_mm_checkheap(...) mm_checkheap(__VA_ARGS__)
#else
# define dbg_printf(...)
# define dbg_mm_checkheap(...)
#endif

/* do not change the following! */
#ifdef DRIVER

/* create aliases for driver tests */
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#endif /* def DRIVER */

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(p) (((size_t)(p) + (ALIGNMENT-1)) & ~0x7)

/* Basic constants and macros */
#define WSIZE       4           // word size (bytes)
#define DSIZE       8           // doubleword size (bytes)
#define CHUNKSIZE   1 << 8	    // initial heap size (bytes)
#define MINIMUM     24          // minimum block size

#define MAX(x, y) ((x) > (y) ? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)  ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)       (*(int *)(p))
#define PUT(p, val)  (*(int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)       ((void *)(bp) - WSIZE)
#define FTRP(bp)       ((void *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)  ((void *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp)  ((void *)(bp) - GET_SIZE(HDRP(bp) - WSIZE))

/* Given block ptr bp, compute address of next and previous free blocks */
#define NEXT_FREEP(bp)(*(void **)(bp + DSIZE))
#define PREV_FREEP(bp)(*(void **)(bp))

/* Internal helper routines */
static void *extendHeap(size_t words);     //Extends heap
static void place(void *bp, size_t asize); //place required block
static void *findFit(size_t asize);        //Find the fit free block
static void *coalesce(void *bp);           //Coalesce free block
static int get_seglist_no(size_t asize);   //Get the list header offset
static void insertAtFront(void *bp);       //Insert free block
//into free list
static void removeBlock(void *bp);         //Remove Block
//from free list
/* Implement Parameter*/
static char *heap_listp = 0;               //Pointer to the heap
#define NUM_OF_LIST 7                      //Free List Number
#define FIRST_BEST_INDEX 5                 //Decide use first or
//Best fit

/* Structure Check*/
static void printBlock(void *bp);          //Print out block
static void checkBlock(void *bp);          //Check block legal

/******************************************************************
 * mm_malloc - Allocate a block with at least size bytes of payload
 
 * Alignment is considered in this part. We have to reserve position
 for header and footer, and previous and next pointer needed if it
 is free, so the minimum size is 24 bytes (4 bytes header, 4 bytes
 footer, 8 bytes previous 8 bytes next pointer) per block.
 
 * If no space was found, we must ask for more memory and place it
 at the head of new heap
 *****************************************************************/
void *mm_malloc(size_t size)
{
	size_t asize;      // aligned block size
	size_t extendsize; // amount to extend heap if no fit
	char *bp;
	// Ignore meaningless requests
	if (size <= 0)
    {
		return NULL;
    }
    // Get assignment
	asize = MAX(ALIGN(size) + DSIZE, MINIMUM);
    
	// Search the free list for a fit
	if ((bp = findFit(asize)))
	{
		place(bp, asize);
		return bp;
	}
	// No fit found. Get more memory and place the block
	extendsize = MAX(asize, CHUNKSIZE);
    
	//return NULL if unable to get heap space
	if ((bp = extendHeap(extendsize/WSIZE)) == NULL)
    {
		return NULL;
    }
	place(bp, asize);
	return bp;
}
/******************************************************************
 * End of mm_malloc
 *****************************************************************/

/******************************************************************
 * mm_free - Free a block
 Adds a block to the free list. Using block pointer, set the
 allocation bits to 0 in header and footer of the block.
 Then, coalesce with adjacent blocks, if applicable.
 This function takes a block pointer as a parameter.
 *****************************************************************/
void mm_free(void *bp)
{
	if(!bp)
    {
        return;     //return if the pointer is NULL
    }
	size_t size = GET_SIZE(HDRP(bp));
	//set header and footer to unallocated
	PUT(HDRP(bp), PACK(size, 0));
	PUT(FTRP(bp), PACK(size, 0));
	coalesce(bp); //coalesce and add the block to the free list
}
/******************************************************************
 * End of mm_free
 *****************************************************************/

/******************************************************************
 * mm_realloc - Reallocate a block
 * This function extends or shrinks an allocated block.
 * If the new size is <= 0, then just free the block.
 * If the new size is the same as the old size,just return the
 old pointer.
 * If the new size is less than the old size, shrink the block
 and copy the front part fit in it.
 * If the new size is greater than the old size, call malloc
 using the new size, copy all of the old data, then call free
 to the original block.
 *****************************************************************/
void *mm_realloc(void *ptr, size_t size)
{
	size_t oldsize;
	void *newptr;
	size_t asize = MAX(ALIGN(size) + DSIZE, MINIMUM);
	// If size <= 0 then this is just free, and we return NULL.
	if(size <= 0)
    {
		free(ptr);
		return 0;
	}
	// If oldptr is NULL, then this is just malloc.
	if(ptr == NULL)
    {
		return malloc(size);
	}
	// Get the size of the original block.
	oldsize = GET_SIZE(HDRP(ptr));
	
	// If the size doesn't need to be changed, return orig pointer
	if (asize == oldsize)
    {
		return ptr;
	}
	// If the size needs to be decreased, shrink the block and
	// return the same pointer
	newptr = malloc(size);
    
	// If realloc() fails the original block is left untouched
	if(!newptr) {
		return 0;
	}
	// Copy the old data.
	if(size < oldsize)
    {
        oldsize = size;
    }
	memcpy(newptr, ptr, oldsize);
    
	// Free the old block.
	free(ptr);
	return newptr;
}
/******************************************************************
 * End of mm_realloc
 *****************************************************************/

/******************************************************************
 * calloc - Allocate the block and set it to zero.
 * call malloc and get every byte 0
 *****************************************************************/
void *calloc (size_t nmemb, size_t size)
{
    size_t bytes = nmemb * size;
    void *newptr;
    
    newptr = malloc(bytes);
    
    memset(newptr, 0, bytes);//set all to 0
    
    return newptr;
}
/******************************************************************
 * End of mm_calloc
 *****************************************************************/



//Below Are Helper Routines
/******************************************************************
 * mm_init - Initialize the memory manager
 
 * The initial heap looks like this:
 
 | Padding | Prelogue | Epilogue|
 4Bytes      TBD       4bytes
 
 * Prelogue
 |Header|    List Reference         |Footer|
 4Bytes   8*(Number Of Freelist)    4Bytes
 
 * List Reference save the value of pointer, pointing to the
 * first available free block in each list
 *
 *****************************************************************/
int mm_init(void)
{
    int INITIAL_SIZE = DSIZE * (NUM_OF_LIST + 2);
    
	// return -1 if unable to get heap space
	if ((heap_listp = mem_sbrk(2*INITIAL_SIZE)) == NULL)
		return -1;
    
	PUT(heap_listp, 0); //Alignment padding
    
	// Initialize header of prelogue
	PUT(heap_listp + WSIZE, PACK(INITIAL_SIZE - DSIZE, 1)); //WSIZE = padding
	
	//Initialize footer of prelogue
	PUT(heap_listp + INITIAL_SIZE - DSIZE, PACK(INITIAL_SIZE - DSIZE, 1));
	
	//Initialize epilogue
	PUT(heap_listp + INITIAL_SIZE - WSIZE, PACK(0, 1));
	
	//Initialize the free list pointer for all free lists
    for (int i = 1; i <= NUM_OF_LIST; i++) {
        PREV_FREEP(heap_listp + i * DSIZE) = NULL;
    }
    
	//return -1 if unable to get first
	if (extendHeap(CHUNKSIZE/WSIZE) == NULL)
    {
		return -1;
    }
	return 0;
}
/******************************************************************
 * End of mm_init
 *****************************************************************/

/******************************************************************
 * extendHeap - Extend heap with free block and return its block pointer
 
 * This function maintains alignment by only allocating an even number.
 
 * We overwrite the epilogue of the previously heap, and put new epilogue
 at the end of the newly added heap.
 *****************************************************************/
static void *extendHeap(size_t words)
{
	char *bp;
	size_t size;
    
	// Allocate an even number of words to maintain alignment
	size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
    
	if (size < MINIMUM)
    {
		size = MINIMUM;
    }
    // Fail to get new heap space, return NULL
	if ((long)(bp = mem_sbrk(size)) == -1)
    {
		return NULL;
    }
	// Initialize free block header/footer and the epilogue header
	PUT(HDRP(bp), PACK(size, 0));         // free block header
	PUT(FTRP(bp), PACK(size, 0));         // free block footer
	PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); // new epilogue
    
    //coalesce with other block if possible
	return coalesce(bp);
}
/******************************************************************
 * End of extendHeap
 *****************************************************************/

/******************************************************************
 * place - Place block of asize bytes at start of free block bp
 
 * This function places the block by comparing asize with the total
 block size, csize.
 
 * If the difference is at least the minimum block
 size, we can split the block into an allocated block and a free block.
 
 * If not, we declare the whole block as allocated to avoid excessive
 external fragmentation.
 
 * This function takes a block pointer to a free block and the
 size of the block we wish to place there.
 ******************************************************************/
static void place(void *bp, size_t asize)
{
	// Gets the size of the whole free block
	size_t csize = GET_SIZE(HDRP(bp));
    
    // If csize - asize >= MINIMUM:
    // (1) Changing the header and footer info for the free
    //      block created from the remaining space
    //      (size = csize-asize, allocated = 0)
    // (2) Coalescing the new free block with adjacent free blocks
	if ((csize - asize) >= MINIMUM)
    {
		PUT(HDRP(bp), PACK(asize, 1));
		PUT(FTRP(bp), PACK(asize, 1));
		removeBlock(bp);
		bp = NEXT_BLKP(bp);
		PUT(HDRP(bp), PACK(csize-asize, 0));
		PUT(FTRP(bp), PACK(csize-asize, 0));
		coalesce(bp);
	}
    // Else:place in the whole block
	else
    {
		PUT(HDRP(bp), PACK(csize, 1));
		PUT(FTRP(bp), PACK(csize, 1));
		removeBlock(bp);
	}
}
/******************************************************************
 * End of place
 *****************************************************************/

/******************************************************************
 * coalesce - boundary tag coalescing.
 
 * If no adjacent free block, just put free block into free block list
 
 * If have adjacent free block, coalesce and then put the new free
 block into free list.
 *****************************************************************/
static void *coalesce(void *bp)
{
	size_t prev_alloc;
	prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp))) || PREV_BLKP(bp) == bp;
	size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
	size_t size = GET_SIZE(HDRP(bp));
    
    // Case 1, no extension, just put block in list and return
    
	// Case 2, extend the block to right */
	if (prev_alloc && !next_alloc)
	{
		size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        //remove right adjacent block from free list
		removeBlock(NEXT_BLKP(bp));
		PUT(HDRP(bp), PACK(size, 0));
		PUT(FTRP(bp), PACK(size, 0));
	}
    
	/* Case 3, extend the block to left */
	else if (!prev_alloc && next_alloc)
	{
		size += GET_SIZE(HDRP(PREV_BLKP(bp)));
		bp = PREV_BLKP(bp);
        //remove left adjacent block from free list
		removeBlock(bp);
		PUT(HDRP(bp), PACK(size, 0));
		PUT(FTRP(bp), PACK(size, 0));
	}
    
	/* Case 4, extend the block in both directions */
	else if (!prev_alloc && !next_alloc)
	{
		size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
        GET_SIZE(HDRP(NEXT_BLKP(bp)));
		removeBlock(PREV_BLKP(bp));
		removeBlock(NEXT_BLKP(bp));
		bp = PREV_BLKP(bp);
		PUT(HDRP(bp), PACK(size, 0));
		PUT(FTRP(bp), PACK(size, 0));
	}
	
	insertAtFront(bp);
	
	return bp;
}

/******************************************************************
 * End of coalesce
 *****************************************************************/
// End of helper Routines




// Key Operations for Dynamic Memory Allocator
/******************************************************************
 * get_seglist_no - get free list Number
 
 * Determines to which free list a block is added
 return the offset against the head of heap
 *****************************************************************/
static int get_seglist_no(size_t asize){
    if (asize <= 24)
        return 1;
    else if (asize <= 192)
        return 2;
    else if (asize <= 480)
        return 3;
    else if (asize <= 3840)
        return 4;
    else if (asize <= 7680)
        return 5;
    else if (asize <= 30720)
        return 6;
    else
        return 7;
}
/******************************************************************
 * End of get_seglist_no
 *****************************************************************/

/******************************************************************
 * findFit - Find a fit for a block with asize bytes
 
 * This function iterates through all free lists
 
 * uses first fits for small blocks and best fit for large blocks
 
 * With the help of insertAtFront function, the selection between
 first fit and best fit is automatically because insertAtFront
 implement different add free block strategy for different block
 size
 ******************************************************************/
static void *findFit(size_t asize)
{
	void *head;
    int offset;
    
    // get offset according to size
    offset = get_seglist_no(asize);
    
    // Search through all the lists
    for (int i = offset; i <= NUM_OF_LIST; i++)
    {
        head = PREV_FREEP(heap_listp + i * DSIZE);
        
        // Search through all the blocks in list
        while (head != NULL) {
            if ((asize <= (size_t)GET_SIZE(HDRP(head))))
            {
                return head;
            }
            head = PREV_FREEP(head);
        }
    }
	return NULL; // No fit, return null
}
/******************************************************************
 * End of get_seglist_no
 *****************************************************************/

/******************************************************************
 * insertAtFront - Inserts a block into free list
 
 * search according to the range of block asize
 
 * for small block, just insert at the front
 
 * for large block, insert according to asize
 in ascending order
 *****************************************************************/
static void insertAtFront(void *bp)
{
    if (bp == NULL) {
        return;
    }
    
    // get the according offset
    int offset = get_seglist_no(GET_SIZE(HDRP(bp)));
    void *free_list = heap_listp + offset * DSIZE;
    
    // get the head of according list
    void *head = PREV_FREEP(free_list);
    
    // if asize is large, put in according to asize
    // in ascending order
    if (offset > FIRST_BEST_INDEX)
    {
        // if list is empty, put it as head
        if (head == NULL)
        {
            PREV_FREEP(free_list) = bp;
            NEXT_FREEP(bp) = free_list;
            PREV_FREEP(bp) = NULL;
            return;
        }
        // put it according to asize in ascending order
        while (PREV_FREEP(head) != NULL)
        {
            if (GET_SIZE(HDRP(head)) > GET_SIZE(HDRP(bp)))
            {
                PREV_FREEP(NEXT_FREEP(head)) = bp;
                NEXT_FREEP(bp) = NEXT_FREEP(head);
                PREV_FREEP(bp) = head;
                NEXT_FREEP(head) = bp;
                return;
            }
            head = PREV_FREEP(head);
            
        }
        
        // if it is the largest in list
        // put it as the tail of list
        PREV_FREEP(head) = bp;
        NEXT_FREEP(bp) = head;
        PREV_FREEP(bp) = NULL;
        return;
    }
    
    else // if it is small, put it in front
    {
        // if list is empty, put it as head
        if (head == NULL)
        {
            PREV_FREEP(free_list) = bp;
            NEXT_FREEP(bp) = free_list;
            PREV_FREEP(bp) = NULL;
            return;
        }
        // put it in the front
        else
        {
            PREV_FREEP(free_list) = bp;
            NEXT_FREEP(bp) = free_list;
            PREV_FREEP(bp) = head;
            NEXT_FREEP(head) = bp;
            return;
        }
        
    }
}

/*****************************************************************
 * End of insertAtFront
 *****************************************************************/

/*****************************************************************
 * removeBlock - remove free block from free list
 *****************************************************************/
static void removeBlock(void *bp)
{
    if (bp == NULL)
    {
        return;
    }
    // if it is not the tail of free list
	if (PREV_FREEP(bp))
    {
        PREV_FREEP(NEXT_FREEP(bp)) = PREV_FREEP(bp);
		NEXT_FREEP(PREV_FREEP(bp)) = NEXT_FREEP(bp);
    }
    // if it is the tail of free list
	else
    {
        PREV_FREEP(NEXT_FREEP(bp)) = NULL;
        NEXT_FREEP(bp) = NULL;
    }
}
/*****************************************************************
 * End of removeBlock
 *****************************************************************/
// End Of Key Operations




// check functions
/******************************************************************
 * mm_checkheap - Check the heap for consistency
 
 * Checks the epilogue and prologue blocks for size and allocation bit.
 
 * Checks the 8-byte address alignment for each block in the free list.
 
 * Checks each free block to see if its next and previous pointers are
 within heap bounds.
 
 * Checks the consistency of header and footer size and allocation bits
 for each free block.
 *****************************************************************/
void mm_checkheap(int verbose)
{
	void *bp = heap_listp + DSIZE; //Points to the first block in the heap
    
	if (verbose)
    {
		printf("Heap (%p):\n", bp);
    }
	// If first block's header's size or allocation bit is wrong,
    // the prologue header is wrong
    
	if ((GET_SIZE(HDRP(bp)) != (NUM_OF_LIST + 1) * DSIZE)
        || !GET_ALLOC(HDRP(bp))){
		printf("Bad prologue header\n");
    }
    // check the consistency of prelogue
	checkBlock(heap_listp + DSIZE);
    
    //Points to the epilogue and check the epilogue
    void *ep = heap_listp + DSIZE * (NUM_OF_LIST + 2) - WSIZE;
    // Print the stats of the last block in the heap
	if (verbose){
        printBlock(ep);
    }
	// If last block's header's size or allocation bit is wrong,
    // the epilogue's header is wrong
	if ((GET_SIZE(HDRP(ep)) != 0) || !(GET_ALLOC(HDRP(ep))))
    {
		printf("Bad epilogue header\n");
    }
    
	// Print the stats of every free block in the free list
    for (int i = 1; i <= NUM_OF_LIST; i++) {
        for (bp = PREV_FREEP(heap_listp + i * DSIZE);
             bp != NULL; bp = PREV_FREEP(bp))
        {
            if (verbose)
            {
                printBlock(bp);
            }
            checkBlock(bp);
        }
        
    }
    
    // Check Every Block In The Heap
    bp = heap_listp + DSIZE;
    while (GET_SIZE(HDRP(bp)) != 0) {
        checkBlock(bp);
        bp = NEXT_BLKP(bp);
    }
    
}
/******************************************************************
 * End of mm_checkheap
 *****************************************************************/



/*****************************************************************
 * printBlock - Prints the details of a block
 
 * This function displays previous and next pointers if the block
 is marked as free.
 
 * This function takes a block pointer (to a block for examination)
 as a parameter.
 *****************************************************************/
static void printBlock(void *bp)
{
	int hsize, halloc, fsize, falloc;
    
	// Basic header and footer information
	hsize = GET_SIZE(HDRP(bp));
	halloc = GET_ALLOC(HDRP(bp));
	fsize = GET_SIZE(FTRP(bp));
	falloc = GET_ALLOC(FTRP(bp));
    
	if (hsize == 0)
	{
		printf("%p: EOL\n", bp);
		return;
	}
	
	// Prints out header and footer info if it's an allocated block.
    // Prints out header and footer info and next and prev info
    // if it's a free block.
	if (halloc)
    {
		printf("%p: header:[%d:%c] footer:[%d:%c]\n", bp,
               hsize, (halloc ? 'a' : 'f'),
               fsize, (falloc ? 'a' : 'f'));
    }
	else
    {
		printf("%p:header:[%d:%c] prev:%p next:%p footer:[%d:%c]\n",
               bp, hsize, (halloc ? 'a' : 'f'), PREV_FREEP(bp),
               NEXT_FREEP(bp), fsize, (falloc ? 'a' : 'f'));
    }
}
/*****************************************************************
 * End of printBlock
 *****************************************************************/

/*****************************************************************
 * checkBlock - Checks a block for consistency
 
 * Checks prev and next pointers to see if they are within heap boundaries.
 
 * Checks for 8-byte alignment.
 
 * Checks header and footer for consistency.
 ******************************************************************/

static void checkBlock(void *bp)
{
    
	// Reports if the next and prev pointers are beyond heap bounds
    // Note!! if the free list head at prelogue are empty when it
    // have no prev_node, it will print error too. THIS situation
    // Can be ignored.
	if (NEXT_FREEP(bp)< mem_heap_lo() || NEXT_FREEP(bp) > mem_heap_hi())
    {
		printf("Error: next pointer %p is not within heap bounds \n"
               , NEXT_FREEP(bp));
    }
	if (PREV_FREEP(bp)< mem_heap_lo() || PREV_FREEP(bp) > mem_heap_hi())
    {
		printf("Error: prev pointer %p is not within heap bounds \n"
               , PREV_FREEP(bp));
    }
    
	// Reports if there isn't 8-byte alignment by checking if the block pointer
    // is divisible by 8.
	if ((size_t)bp % 8)
    {
		printf("Error: %p is not doubleword aligned\n", bp);
    }
    
	// Reports if the header information does not match the footer information
	if (GET(HDRP(bp)) != GET(FTRP(bp)))
    {
		printf("Error: header does not match footer\n");
    }
}
/*****************************************************************
 * End of checkBlock
 *****************************************************************/
// End of check functions



