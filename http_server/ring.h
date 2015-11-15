#include <stdio.h>
#include <stdlib.h>
#define BUFFER_LENGTH 256

struct ring* rb_create(void);
void rb_delete(struct ring *rb);
int rb_isempty(struct ring *rb);
int rb_isfull(struct ring *rb);
int rb_enqueue(struct ring *rb, void *value);
void* rb_dequeue(struct ring *rb);

/*
* This is the ring struct
* it holds an array to store the ring buffer
* also the head which points to the first element
* and the tail which points to the last element
 */
struct ring{
	void* rb[BUFFER_LENGTH];
	int head;
	int tail;
};

/*
* Initializes the ring buffer
 */
struct ring* rb_create(void){
	struct ring* rb = malloc(sizeof(struct ring));
	rb->head = 0;
	rb->tail = 0;
}

/*
* adds and element to the array at the tail
 */
int rb_enqueue(struct ring *rb, void *value){
	if(rb_isfull(rb)){
		return NULL;
	}
	rb->rb[(rb->tail)] = value;
	rb->tail = (rb->tail + 1) % (BUFFER_LENGTH);
	return -1;
}

/*
* removes the element at the head
 */
void* rb_dequeue(struct ring *rb){
	if(rb_isempty(rb)){
		return NULL;
	}
	void *val = rb->rb[rb->head];
	rb->rb[rb->head] = NULL;
	rb->head = (rb->head+1) % BUFFER_LENGTH;
	return val;
}

/*
* checks the ring buffer to see if it if full
 */
int rb_isfull(struct ring *rb){
	return (rb->tail + 1) % BUFFER_LENGTH == rb->head;
}

/*
* checks to see if the ring buffer is empty
 */
int rb_isempty(struct ring *rb){
	if(rb->tail == rb->head){
		return 1;
	}else{
		return 0;
	}
}

/*
* clears the ring buffer
 */
void rb_delete(struct ring *rb){
	while(rb_dequeue(rb) != NULL){
		rb_dequeue(rb);
	}
	free(rb);
}

/*
* prints the ring buffer
 */
void printer(struct ring *rb){
	int pos = rb->head;
	do{
		printf("%d\n",rb->rb[pos]);
		pos++;
		if (pos == BUFFER_LENGTH){
			pos = 0;
		}
	}while(pos != rb->tail);
}
