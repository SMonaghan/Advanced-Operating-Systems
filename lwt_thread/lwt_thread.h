#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <sys/mman.h>
//#include <unistd.h>

#define STACK_SIZE (8 * 4096)//Size of the stack
#define LWT_NULL NULL//This is for the scheduling
#define STACK_COUNT 256
unsigned int active_thread_count = 0;
unsigned int global_id = 0;
void * stack_start;

struct linked_list{
	struct Node* head;
	struct Node* tail;
};

struct Node{
	struct Node* next;
	int available;
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
	flag_t flag;
	void *value;
} *lwt_t;

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
void * stack_assign(struct linked_list* stack);
void add_stack(struct linked_list* stack, int id);

typedef void *(*lwt_fn_t)(void *);
lwt_t lwt_create(lwt_fn_t fn, void *data);
lwt_t lwt_current(void);
void * lwt_join(lwt_t thread);
void * stack_create(void);
void lwt_die(void * ret);
int lwt_yield(lwt_t thread);
int lwt_id(lwt_t thread);
int lwt_info(lwt_info_t t);

struct linked_list* stk;// = malloc(sizeof(struct linked_list));

int generate_id(void){
	printf("start\n");
	active_thread_count = active_thread_count + 1;
	printf("counter incremented %d \n",active_thread_count);
	return active_thread_count;
}
lwt_t lwt_create(lwt_fn_t fn, void *data){
	printf("Start\n");	
	lwt_t lwt_thd = malloc(sizeof(struct thread));
	printf("Malloced!!!!!!!\n");	

	lwt_thd->id = generate_id();
	lwt_thd->stack = stack_assign(stk);//stack_start + (STACK_SIZE * (lwt_thd->id));
	printf("Stack Allocated!!!!!!! %u\n",lwt_thd->stack);	
	//lwt_thd->id = global_id;//generate_id();
	global_id++;
	printf("ID Generated!!!!!!!\n");
	void* sp = lwt_thd->stack + STACK_SIZE;
	//lwt_thd->sp = (unsigned int) lwt_thd->stack + STACK_SIZE;

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

	return lwt_thd;

}

void * lwt_join(lwt_t thread){

}

void lwt_die(void * ret){

}

int lwt_yield(lwt_t thread){

}

lwt_t lwt_current(void){
	return active_thread;
}

int lwt_id(lwt_t thread){
	return thread->id;
}

int lwt_info(lwt_info_t t){

}

void __lwt_schedule(void){
	lwt_t next_thread = active_thread;
	/*next_thread = ?*/
	/*put stuff here*/
	active_thread = next_thread;


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
	stack_assign(stk);
}

void __lwt_stack_return(void *stk){

}

void * stack_create(void){
	stack_start = aligned_alloc(STACK_SIZE, STACK_SIZE * STACK_COUNT);
	stk = malloc(sizeof(struct linked_list));
	int i = 1;
	for (i; i < STACK_COUNT; i++){
		add_stack(stk, i);
	}
	return stack_start;
}

struct linked_list* list_create(void){
	stk->head = NULL;
	return stk;
}

void add_stack(struct linked_list* stack, int id){
	struct Node* node = malloc(sizeof(struct Node));
	node->next = stack->head;
	if(stack->head == NULL){
		stack->head = node;
		stack->tail = node;
	}
	stack->head = node;
	node->value = stack_start + (STACK_SIZE * id);
	node->available = 0;
}

void * stack_assign(struct linked_list* stack){
	void * ret = stack->head->value;
	assert(stack->head->available == 0);
	/*if(stack->head->available != 0){
		return -1;
	}*/
	stack->head->available = 1;
	stack->tail->next = stack->head;
	stack->head = stack->head->next;
	stack->tail->next = NULL;
	return ret;
}

