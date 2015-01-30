/*
* Stephen Monaghan
* Advanced OS
*/

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

struct linked_list{
	struct Node* head;
	struct Node* tail;
};

struct Node{
	void *value;
	struct Node* next;
	struct Node* prev;
};

struct linked_list* ll_create(void);
//struct Node* ll_find_least_ref_count(struct linked_list *ll);
int ll_length(struct linked_list *ll);
int ll_contains(struct linked_list *ll, void *value);
void* ll_remove_first(struct linked_list *ll);
void ll_destroy(struct linked_list *ll);
void ll_add(struct linked_list *ll, void *value);
//void ll_reorder(struct linked_list *ll);
void print(struct linked_list *ll);

struct linked_list* ll_create(void){
	struct linked_list* ll = malloc(sizeof(struct linked_list));//creates a linked list
	ll->head = NULL;//sets the value of the head to null
	ll->tail = NULL;
	return ll;
}

void ll_remove(struct linked_list *ll, struct Node* rem){
     struct Node* temp = ll->head;
     while(temp != rem){
         if(temp == ll->tail) return;
         temp = temp->next;
     }

     if(ll->head == rem && ll->tail == rem){
         ll->head = NULL;
         ll->tail = NULL;
         return;
     }

     if(ll->head == rem) ll->head = rem->next;
     if(ll->tail == rem) ll->tail = rem->prev;
     
     rem->prev->next = rem->next;
     rem->next->prev = rem->prev;
}


void ll_add(struct linked_list *ll, void *value){
	struct Node* newnode = malloc(sizeof(struct Node));//creates a temp node
	newnode->next = ll->head;
	newnode->prev = ll->tail;
	if (ll->head != NULL){//if there is a  node in the current list, it will set the next node to be the current first node
		ll->head->prev = newnode;//sets the old first node's prev to the new node
		ll->tail->next = newnode;//sets the tails next to the new node
	}else{
		ll->head = newnode;//sets the head to the new node
	}
	ll->tail = newnode;//sets the new node to the free node
	newnode->value = value;//sets the value of the new node to value
}

/*
* this will remove the first node (only if empty though)
 */
void* ll_remove_first(struct linked_list *ll){
	if(ll->head==NULL) return NULL;
	else{
		void* value = ll->head->value;

		if(ll->head==ll->tail){
			free(ll->head);
			ll->head = NULL;
			ll->tail = NULL;
		}
		else
		{
			free(ll->head);
			ll->head = ll->head->next;
			ll->tail->next = ll->head;
			ll->head->prev = ll->tail;
		}
		return value;
	}

}
/*
* moves the node with the least references to the front
 */
//void ll_reorder(struct linked_list *ll){
/*	struct Node *node = ll_find_least_ref_count(ll);
	node->prev->next = node->next;
	node->next->prev = node->prev;
	node->next = ll->head;
	ll->head->prev = node;
	ll->tail = node;*/
//}
/*
* finds the node/slab with the least number of references aka least filled
 */
/*struct Node* ll_find_least_ref_count(struct linked_list *ll){
	struct kmem_slab* slab_struct = malloc(sizeof(struct kmem_slab));
	struct Node* finder = malloc(sizeof(struct Node));
	int ref = 9999999;

	finder = ll->head;

	if(finder == NULL){
		printf("This list is empty\n");
		return;
	}
	while(finder != NULL){
		slab_struct = finder->value;
		if (slab_struct->slab_refcnt == 0) return finder;
		if (slab_struct->slab_refcnt < ref) ref = slab_struct->slab_refcnt;
		finder = finder->next;
	}
	printf("Ref Count: %d", ref);
	slab_struct = finder->value;
	while(slab_struct->slab_refcnt != ref){
		slab_struct = finder->value;
		finder = finder->prev;
	}
	return finder;

}
*/

/*
* destroys the list by removing all the first nodes
*/
void ll_destroy(struct linked_list *ll){
	while(!ll_remove_first(ll));{
		ll_remove_first(ll);
	}
	free(ll);
}

/*
* determines if the list contains a specific value
*/
int ll_contains(struct linked_list *ll, void *value){
	struct Node* search = malloc(sizeof(struct Node));
	search = ll->head;//sets search to the head of the list
	int counter = 1;

	if(ll->head == NULL){//this means the list is empty
		printf("This list is empty\n");
		return 0;
	}
	while(search->value != value && search->next != ll->head){//this traverses the list looking for the value
		counter++;
		search = search->next;
	}
	if(search->value != value) return 0;
	return counter;

}

/*
* determines the length of the list
*/
int ll_length(struct linked_list *ll){
	struct Node* search = malloc(sizeof(struct Node));
	search = ll->head;//sets this node to the first one
	int counter = 1;

	if(ll->head == NULL){//this means the list is empty
		printf("This list is empty\n");
		return 0;
	}
	while(search->next != ll->head){//goes through the whole list and increments a counter
		counter++;
		search = search->next;
	}
	return counter;
}

/*
* prints the list
*/
void ll_print(struct linked_list *ll){
	struct Node* search = ll->head;
	printf("List: \n");
	while(search != NULL){
        	printf("%d\n", search->value);
        	if(search == ll->tail) break;
        	search = search->next;
     	}
	printf("END\n");

}

