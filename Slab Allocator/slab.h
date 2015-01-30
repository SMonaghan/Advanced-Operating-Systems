/*
* Stephen Monaghan
* Advanced OS
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "list.h"
#include "uthash.h"

#define PAGE_SIZE 4096
/*STRUCTS*/
struct kmem_cache{
	char* name;
	size_t size;
	struct linked_list* slab_list;
	struct Node* free_slab;
	struct kmem_bufctl* buffhash;
	int large;
	int  offSet;
};

struct kmem_slab{
	int allocated; 
	int max_objs;
	struct linked_list* freelist;
};

struct kmem_bufctl{
	struct kmem_slab* slab;
	void* buffer;
	UT_hash_handle hh;
};

/*DECLARATIONS*/

void kmem_init();
struct kmem_cache *kmem_cache_create(char *name, size_t size);
void *kmem_cache_alloc(struct kmem_cache *cp);
void kmem_cache_grow(struct kmem_cache *cp);
void kmem_cache_free(struct kmem_cache *cp, void *buf);
void kmem_cache_reap(struct kmem_cache *cp);
void kmem_cache_destroy(struct kmem_cache *cp);
void print_cache_info(struct kmem_cache *cp);
void print_slab_info(struct kmem_slab *sp);

struct kmem_cache* slab_structs = NULL;
struct kmem_cache* ctl_structs = NULL;

/*INITIALIZATION / CREATION SECTION*/

/*
* this initializes the two structs
*/
void kmem_init(){
	slab_structs = kmem_cache_create("slab_structs", sizeof(struct kmem_slab));
	ctl_structs = kmem_cache_create("ctl_structs", sizeof(struct kmem_bufctl));
}

/*
* this creates the kmem_cache struct and initializes the values
*/
struct kmem_cache *kmem_cache_create(char *name, size_t size){
	struct kmem_cache* cache = malloc(sizeof(struct kmem_cache));//creates a temp cache
	cache->name = name;//sets the name to the supplied name
	cache->size = size;//sets the size to the supplied size
	cache->slab_list = ll_create();//initializes the slab list with a linked list
	cache->free_slab = NULL;
	cache->buffhash = NULL;
	cache->offSet = -1;
	cache->large = (size > (PAGE_SIZE / 8));//determines if the size is too large to be managed normally

	return cache;//returns the newly created cache
}

/*GROWTH SECTION*/

/*
* this allocates the memory for the cache
*/
void *kmem_cache_alloc(struct kmem_cache *cp){
	struct kmem_slab* slab;//creates a slab
	if(cp->free_slab != NULL){//if there is a current slab it just adds the slab to the list
	       slab = cp->free_slab->value;
	}
	if(cp->free_slab == NULL || slab->allocated == slab->max_objs){//if the free slab list is empty or full it grows the cache and then retries allocating it
		kmem_cache_grow(cp);
		return kmem_cache_alloc(cp);
	}
	else{//if it is not empty or full it adds one to the allocated slabs and then removes the first node in the free list of slabs
		slab->allocated++;
		return ll_remove_first(slab->freelist);
	}
}

/*
* this increases the number of slabs in the cache
*/
void kmem_cache_grow(struct kmem_cache *cp){
	struct kmem_slab slab;//slab
	struct kmem_slab* free_slab;//slab
	slab.freelist = ll_create();//adds a free list to the new slab
	slab.allocated = 0;//sets the number of allocated to 0 (because its new)
	if(cp->large){//if it is large it requires special processing
		slab.max_objs = 1;//it can only have one object because of its size (wich oppresses the slab!!!!!!!!!!!)
		if(slab_structs==NULL || ctl_structs==NULL) kmem_init();//if the slab and control structs do not exist it calls the init function above
		struct kmem_slab* slab_mem = kmem_cache_alloc(slab_structs);//allocates memory for the slab memory structs
		memcpy(slab_mem, &slab, sizeof(struct kmem_slab));//adds the memory of the slab to the slab

		int mem_alloc = PAGE_SIZE;//amount of memory to allocate
		while(mem_alloc < cp->size) mem_alloc+=PAGE_SIZE;//makes sure it can contain the object, and if not adds more until it can one page at a time

		void* buf = malloc(mem_alloc);//sets the buffer
		struct kmem_bufctl* buf_ctl = kmem_cache_alloc(ctl_structs);//creates a buffer control struct
		buf_ctl->slab = slab_mem;//sets the slab in it to the new slab mem
		buf_ctl->buffer = buf;//sets the buffer to the new buffer
		HASH_ADD_INT(cp->buffhash, buffer, buf_ctl);//adds the buffer to the hash
		printf("added %d to cache with %d\n", buf_ctl, buf);//informs user that the buffer was added to cache
		ll_add(slab_mem->freelist, buf);//adds the buffer to the free list
		ll_add(cp->slab_list, slab_mem);//adds the slab to the list
		free_slab = slab_mem;//sets free slab to slab mem

	}else{
		slab.max_objs = (PAGE_SIZE-sizeof(struct kmem_slab))/cp->size;//sets the max number of objects to a page size divided by the size of the objects
		void* page = malloc(PAGE_SIZE);//allocates a page of memory
		int i;
		for(i = 0; i < (PAGE_SIZE-sizeof(struct kmem_slab)); i += cp->size){//adds new objects to the slab free list
			ll_add(slab.freelist, page+i);
		}
		memcpy(page+PAGE_SIZE-sizeof(struct kmem_slab), &slab, sizeof(struct kmem_slab));//copies the memory
		ll_add(cp->slab_list, page+PAGE_SIZE-sizeof(struct kmem_slab));//adds the memory to the slab list
		free_slab = page+PAGE_SIZE-sizeof(struct kmem_slab);//sets the free slab to the memory plus a page size - the struct size
	}
	struct Node* find = cp->slab_list->head;//a searching node
	while(find->value != free_slab) find = find->next;//looks through the slab list
	cp->free_slab = find;//sets the free slab to the found slab
	cp->offSet = (int) (free_slab) % 4096;//sets the offset to the remainder of the size and a page size
	printf("Cache %s expanded, offset %d\n", cp->name, cp->offSet);//names the expanded cache and its offset
	ll_print(cp->slab_list);//prints the list of slabs
}

/*REDUCTION SECTION*/

/*
* this frees the cache
*/
void kmem_cache_free(struct kmem_cache *cp, void *buf){
	struct kmem_slab* slab;
	if(cp->large){//if the cache is of a large size it requires a special handler
		struct kmem_bufctl* ctl;//creates a control buffer
		int bufadr = buf;//sets the address of the buffer
		HASH_FIND_INT(cp->buffhash, &bufadr, ctl);//finds the buffer in the hash list
		slab = ctl->slab;//sets the slab to the control buffers slab
	}else{//it is a normal sized cache
		struct Node* tmp = cp->slab_list->head;//temp node
		while(((int) tmp->value - (int) buf) > 4196 || ((int) tmp->value - (int)buf) < 0){//checks the values of the temp nodes in the slab is
		       tmp = tmp->next;
		}
		slab = tmp->value;
	}
	slab->allocated--;//removal of an allocated slab (number wise)
	ll_add(slab->freelist, buf);//adds the newly freed slab back to the free list so it can be used
	if(slab->allocated==0) kmem_cache_reap(cp);//if there are no allocated portions of the slab it reaps (removes it)
}

/*
* this removes empty slabs from the cache
*/
void kmem_cache_reap(struct kmem_cache *cp){
	struct Node* empty_slab = cp->slab_list->head;//searching node for slab list
	struct kmem_slab* slab = NULL;
	int slabAlloc = -1;

	while(slabAlloc != 0){
		slab = empty_slab->value;//sets a slab equal to the slab the temp is on
		slabAlloc = slab->allocated;//sets the value of slabAlloc to whether the slab is allocated or not
		empty_slab = empty_slab->next;//moves the temp slab
	} 
	empty_slab = empty_slab->prev;//moves the temp back one

	if(ll_length(cp->slab_list)==1){//checks to see if the length of the slab list is one
	       cp->free_slab=NULL;
	
	}else if(cp->free_slab==empty_slab){//checks to see if the slab list is composed of the same node as the temp
	       	cp->free_slab = empty_slab->prev;
	
	}

	if(cp->large){//handles the condition that the cache is of a large size
		struct kmem_bufctl* ctl_buf;//creates a buffer control structure
		int adr = slab->freelist->head->value;//looks at the address of the slab 
		HASH_FIND_INT(cp->buffhash, &adr, ctl_buf);//looks in the hash list for that address
		HASH_DEL(cp->buffhash, ctl_buf);//delets that slab
		kmem_cache_free(ctl_structs, ctl_buf);//removes the control buffer
		kmem_cache_free(slab_structs, empty_slab->value);//removes the slab structures
		ll_destroy(slab->freelist);//destroys the slabs free list
	}else{
		ll_destroy(slab->freelist);//destroys the free list if it is not large
	}
	ll_remove(cp->slab_list, empty_slab);//removes the specified slab from the slab list
	ll_print(cp->slab_list);//prints the remaining slabs in the list
}

/*
* this removes the cache entirely
*/
void kmem_cache_destroy(struct kmem_cache *cp){
	if(cp->slab_list == 0){//if the slab list is empty it will free the memory of the cache
		free(cp);
	}
}

/*THE PRINTING OF STUFF SECTION*/

/*
* this returns information to the user about the cache
*/ 
void print_cache_info(struct kmem_cache *cp){
	printf("\nName: %s\n", cp->name);//this returns the name of the cache supplied
	printf("Size of objects: %d\n", (int) cp->size);//this returns how large each object in the slab is
	if(cp->free_slab != NULL){
	       	printf("Free: %d\n", cp->free_slab->value);//returns the value of the free slab
	}
	printf("Large? %d\n", cp->large);//determines if the slab is large enough to fit into special qualifications
	printf("Slab information: \n");
	struct Node* tmp = cp->slab_list->head;
	while(tmp != NULL){//checks to be sure that there are slabs in the cache
		print_slab_info(tmp->value);
		if(tmp == cp->slab_list->tail) return;//checks to see if this is the last slab in the list
		tmp = tmp->next;//goes to the next slab
	}
	printf("None\n");//if there is no information to be supplied
}

/*
* this prints the info about the slab for the user to see
*/
void print_slab_info(struct kmem_slab *sp){
	printf("Location: %d\n", sp);//this is the address of the supplied slab
	printf("Free: ");//this is the free objects in the free list
	ll_print(sp->freelist);
	printf("allocated objects: %d\n", sp->allocated);//this is the number of allocated objects in the slab
	printf("potential objects: %d\n", sp->max_objs);//this is how many objects could be allocated in the slab
}
