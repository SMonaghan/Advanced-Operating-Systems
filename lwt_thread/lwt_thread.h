#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <sys/mman.h>
//#include <unistd.h>

#define STACK_SIZE (8 * 4096)//Size of the stack
#define LWT_NULL NULL//This is for the scheduling
#define STACK_COUNT 256
unsigned int global_id = 0;
void * stack_start;

struct linked_list{
	struct Node* head;
	struct Node* tail;
};

struct Node{
	struct Node* next;
	struct Node* prev;	
	void * value;
};

typedef enum {
	RUNNING,
	WAITING,
	READY,
} flag_t;

typedef struct thread {
	void * stack;
	unsigned int id;
	void * sp;
	struct thread *parent;
	struct thread *child;
	struct thread *next;
	struct thread *prev;
	flag_t flag;
	void *value;
} *lwt_t;

struct queue{
	struct thread *head;
	struct thread *tail;
};

typedef enum {
	LWT_INFO_NTHD_RUNNABLE,
	LWT_INFO_NTHD_BLOCKED,
	LWT_INFO_NTHD_ZOMBIES,
} lwt_info_t;

static lwt_t active_thread = NULL;

void __lwt_schedule(void);
void __lwt_dispatch(lwt_t next, lwt_t current);
void __lwt_trampoline(void);
void * __lwt_stack_get(void);
void __lwt_stack_return(void *stk);

struct linked_list* list_create(void);
void add_stack(struct linked_list* stack, void * val);
void stack_deallocate(void * value);

typedef void *(*lwt_fn_t)(void *);
lwt_t lwt_create(lwt_fn_t fn, void *data);
lwt_t lwt_current(void);
void * lwt_join(lwt_t thread);
void * stack_create(void);
void lwt_die(void * ret);
int lwt_yield(lwt_t thread);
int lwt_id(lwt_t thread);
int lwt_info(lwt_info_t t);

struct linked_list* stk;
struct queue* run_queue;
struct thread* look_thread;

lwt_t lwt_create(lwt_fn_t fn, void *data){
	printf("Start\n");	
	lwt_t lwt_thd = malloc(sizeof(struct thread));
	printf("Malloced!!!!!!!\n");	

	lwt_thd->id = global_id;
	lwt_thd->stack = __lwt_stack_get();
	printf("Stack Allocated!!!!!!! %u\n",lwt_thd->stack);	
	global_id++;
	printf("ID Generated!!!!!!!\n");
	void* sp = lwt_thd->stack + STACK_SIZE;

	*((int*) sp-1) = __lwt_trampoline;
	*((int*) sp-2) = sp;

	printf("two things on the stack %u: %u at %u and %u at %u\n", sp, *((int*) sp-1), (int*)sp-1,  *((int*) sp-2), (int*)sp-2);
	lwt_thd->sp = sp-32;
	printf("Stack Pointer set!!!!!!!\n");	
	//lwt_thd->parent = lwt_current();
	//printf("Parent Decided!!!!!!!\n");	
	lwt_thd->child = NULL;
	printf("Child Set!!!!!!!\n");	
	lwt_thd->value = NULL;
	printf("Value Set!!!!!!!\n");
	lwt_thd->flag = RUNNING;
	if(run_queue->tail != NULL){
	       run_queue->tail->next = lwt_thd;
	       lwt_thd->prev = run_queue->tail;
	}
	run_queue->tail = lwt_thd;
	if(run_queue->head == NULL){
	       run_queue->head = lwt_thd;
	}
		
	return lwt_thd;

}

void * lwt_join(lwt_t thread){

}

void lwt_die(void * ret){

}

int lwt_yield(lwt_t thread){
	assert(thread != lwt_current());
	if(thread == LWT_NULL){
	       __lwt_schedule;
	       return 1;
	}
	__lwt_dispatch(run_queue->head, thread);
	
	thread->prev->next = thread->next;//move thread out of queue
	thread->next->prev = thread->prev;//move thread out of queue
	/*thread is now out of the queue*/

	thread->next = run_queue->head->next;//sets the next of thread to the thread following the old head
	thread->next->prev = thread;//sets the second in the queue's prev to the this thread
	/*thread is now the head in all but name*/

	run_queue->tail->next = run_queue->head;//sets the tail's next to the old current making it new tail in all but title
	run_queue->head->prev = run_queue->tail;//sets the old currents prev to the old tail
	run_queue->tail = run_queue->head;//sets the title of tail to the old/ current head
	run_queue->head->next = NULL;//sets the old currents next to null so it no longer points anywhere the only thing refering to it is run_queue->head
	/*the old head is now the tail*/

	run_queue->head = thread;//sets the head to the thread
	/*thread is now the head of the queue*/


}

lwt_t lwt_current(void){
	return run_queue->head;
}

int lwt_id(lwt_t thread){
	return thread->id;
}

int lwt_info(lwt_info_t t){

}

void __lwt_schedule(void){
	__lwt_dispatch(run_queue->head, run_queue->head->next);
	run_queue->tail->next = run_queue->head;
	run_queue->head->prev = run_queue->tail;
	run_queue->tail = run_queue->head;
	run_queue->head = run_queue->head->next;
	run_queue->tail->next = NULL;
}

void __lwt_dispatch(lwt_t current, lwt_t next){

	asm volatile ("pushl %eax \n\t"
			"pushl %ebx \n\t"
			"pushl %ecx \n\t"
			"pushl %edx \n\t"
			"pushl %esi \n\t"
			"pushl %edi \n\t");

	asm volatile ("movl %%esp, %0 \n\t"
			"movl %1, %%esp \n\t"
			:"=r" (current->sp)
			:"r" (next->sp)
			:"memory");

	int i, j;
	for(i = 0; i<=8; i++)
		printf("stack %u: %u: %u\n", next->sp, (int*) next->sp+i, *((int*) next->sp+i));	


	asm volatile ("popl %edi \n\t"
			"popl %esi \n\t"
			"popl %edx \n\t"
			"popl %ecx \n\t"
			"popl %ebx \n\t"
			"popl %eax \n\t"
			"popl %ebp \n\t"
			"ret");

	asm volatile("movl %%esp, %0 \n\t"
			"mov %%edi, %1 \n\t"
			: "=r" (i), "=r" (j)
			: : );

	printf("stack pointer: %u, edi: %u\n", i, j);

	asm volatile("jmp %edi\n\t");

}

void __lwt_trampoline(void){
	printf("boing\n");
	exit(0);
}

void * __lwt_stack_get(void){
	assert(stk->head != NULL);
	void * ret = stk->head->value;
	stk->head = stk->head->next;
	return ret;
}

void __lwt_stack_return(void *stck){
	lwt_t cur = lwt_current();
	run_queue->head = run_queue->head->next;
	run_queue->head->prev = NULL;
	add_stack(stk, cur->stack);

}

void * stack_create(void){
	stack_start = aligned_alloc(STACK_SIZE, STACK_SIZE * STACK_COUNT);
	stk = malloc(sizeof(struct linked_list));
	run_queue = malloc(sizeof(struct queue));
	look_thread = malloc(sizeof(struct thread));
	int i = 1;
	for (i; i < STACK_COUNT; i++){
		add_stack(stk, (stack_start + (STACK_SIZE * i)));
	}
	return stack_start;
}

struct linked_list* list_create(void){
	stk->head = NULL;
	return stk;
}

void add_stack(struct linked_list* stack, void * val){
	struct Node* newnode = malloc(sizeof(struct Node));//creates a temp node
	if (stack->head != NULL){//if there is a  node in the current list, it will set the next node to be the current first node
		//stack->head->prev = newnode;//sets the old first node's prev to the new node
		stack->tail->next = newnode;//sets the tails next to the new node
	}else{
		stack->head = newnode;//sets the head to the new node
	}
	stack->tail = newnode;//sets the new node to the free node
	newnode->value = val;
}

