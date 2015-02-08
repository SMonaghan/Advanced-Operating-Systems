#include <stdlib.h>
#include <stdio.h>

struct linked_list{
	struct Node* head;
};

struct Node{
	void *value;
	struct Node* next;
};

struct linked_list* ll_create(void);
void ll_destroy(struct linked_list *ll);
void ll_add(struct linked_list *ll, void *value);
int ll_length(struct linked_list *ll);
void* ll_remove_first(struct linked_list *ll);
int ll_contains(struct linked_list *ll, void *value);
void printList(struct linked_list *ll);

struct linked_list* createList(void){
	struct linked_list* ll = malloc(sizeof(struct linked_list));//creates a linked list
	ll->head = NULL;//sets the value of the head to null
	return ll;
}

void ll_add(struct linked_list *ll, void *value){
	struct Node* newnode = malloc(sizeof(struct Node));//creates a temp node
	if (ll->head != NULL){//if there is a  node in the current list, it will set the next node to be the current first node
		newnode->next = ll->head;//set the next value of the node to the node at the front
	}
	ll->head = newnode;//sets the head to the new node
	newnode->value = value;//sets the value of the new node to value
}

void* ll_remove_first(struct linked_list *ll){
	if(ll->head == NULL){//this means there are no nodes in the list
		printf("There is no node to delete\n");
		return;
	}
	struct Node* del = ll->head;//creates a temp node that is the same 
	void *ret = ll->head->value;//sets the value of ret to the value of the first node
	ll->head = ll->head->next;//sets the current first node to 
	free(del);
	return ret;
}

void printList(struct linked_list *ll){
	struct Node* printer = malloc(sizeof(struct Node));
	printer = ll->head;
	int counter = 1;
	if(printer == NULL){
		printf("This list is empty\n");
		return;
	}
	printf("LIST NAME\n");
	while(printer!= NULL){
		printf("Node %d\n", counter);
		printf("Price: %d\n", printer->value);
		printf("--------------\n");
		counter++;
		printer = printer->next;
	}
}

void ll_destroy(struct linked_list *ll){
	struct Node* search = malloc(sizeof(struct Node));//creates a node to search the list
	struct Node* del = malloc(sizeof(struct Node));//this node deletes it

	del = ll->head;//sets del to the first node

	while(del != NULL){//while there are nodes to delete
		search = del->next;//search is the next node
		free(del);//deletes the node at del
		del = search;//sets delete to search
	}
	free(search);
	free(del);
	ll->head = NULL;
}


int ll_contains(struct linked_list *ll, void *value){
	struct Node* search = malloc(sizeof(struct Node));
	search = ll->head;//sets search to the head of the list
	int counter = 1;

	if(search == NULL){//this means the list is empty
		printf("This list is empty\n");
		return 0;
	}
	while(search->value != value && search->next != NULL){//this traverses the list looking for the value
		counter++;
		search = search->next;
	}
	while(search->value != value && search->next == NULL){//this means that the list does not contain the value
		printf("This list does not contain a matching value\n");
		return 0;
	}
	return counter;

}
int ll_length(struct linked_list *ll){
	struct Node* search = malloc(sizeof(struct Node));
	search = ll->head;//sets this node to the first one
	int counter = 1;

	if(search == NULL){//this means the list is empty
		printf("This list is empty\n");
		return 0;
	}
	while(search->next != NULL){//goes through the whole list and increments a counter
		counter++;
		search = search->next;
	}
	return counter;
}
