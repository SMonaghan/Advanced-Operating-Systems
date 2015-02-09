#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <sys/mman.h>
#include <malloc.h>
//#include <unistd.h>

#define STACK_SIZE (8 * 4096)//Size of the stack
#define LWT_NULL NULL//This is for the scheduling
#define MAX_THREADS 4

typedef void *(*lwt_fn_t)(void *);
unsigned int * active_thread_count = 0;
unsigned int global_id = 1;

typedef struct thread {
	void * stack;
	unsigned int id;
	void * sp;
	struct thread *next, *prev;
	void *param, *ret;
	lwt_fn_t function;
} *lwt_t;

typedef enum {
	LWT_INFO_NTHD_RUNNABLE,
	LWT_INFO_NTHD_BLOCKED,
	LWT_INFO_NTHD_ZOMBIES,
} lwt_info_t;

static lwt_t rq_head, rq_tail;
static lwt_t pool_head;

void __lwt_schedule(void);
void __lwt_trampoline(void);
void __lwt_dispatch(lwt_t next, lwt_t current);
void * __lwt_stack_get(void);
void __lwt_stack_return(void *stk);

lwt_t lwt_create(lwt_fn_t fn, void *data);
lwt_t lwt_current(void);
void * lwt_join(lwt_t thread);
void lwt_die(void * ret);
int lwt_yield(lwt_t thread);
int lwt_id(lwt_t thread);
int lwt_info(lwt_info_t t);

lwt_t lwt_create(lwt_fn_t fn, void *data){
	printf("Start\n");	
	lwt_t lwt_thd = __lwt_stack_get();
	printf("Stack Allocated!!!!!!! %u\n", lwt_thd);	
	lwt_thd->id = global_id;
	global_id++;
	printf("ID Generated!!!!!!! %d\n", lwt_thd->id);
	void* sp = (int) lwt_thd + STACK_SIZE-4;
	//lwt_thd->sp = (unsigned int) lwt_thd->stack + STACK_SIZE;

	lwt_thd->param = data;
	lwt_thd->function = fn;

	printf("more data set, sp at %u\n", sp);
	*((int*) sp-1) = __lwt_trampoline;
	*((int*) sp-2) = sp;

	printf("two things on the stack %u: %u at %u and %u at %u\n", sp, *((int*) sp-1), (int*)sp-1,  *((int*) sp-2), (int*)sp-2);
	lwt_thd->sp = sp-32;
	printf("Stack Pointer set!!!!!!!\n");	
	lwt_thd->ret = NULL;
	printf("Value Set!!!!!!!\n");	

	lwt_thd->next = NULL;
	if(rq_head == NULL){
		rq_head = lwt_thd;
	}else{
		rq_tail->next = lwt_thd;
		lwt_thd->prev = rq_tail;
	}
	rq_tail = lwt_thd;

	return lwt_thd;
}

void * lwt_join(lwt_t thread){

}

void lwt_die(void * ret){
	printf("dying\n");
}

int lwt_yield(lwt_t thread){
	lwt_t current = lwt_current();
	assert(rq_head == current);

	if(current->next!=NULL){
		rq_head = current->next;
		current->next=NULL;
		current->prev=rq_tail;
	}
	rq_tail = current;

	if(thread != LWT_NULL){
		if(thread->id < 1 || thread->id > global_id) return 0;

		if(thread!=rq_head){
			if(thread!=rq_tail){
				thread->prev->next = thread->next;
				thread->next->prev = thread->prev;
			}
			else{
				rq_tail = thread->prev;
				rq_tail->next = NULL;
			}			
			thread->prev = NULL;
			thread->next = rq_head;
			rq_head = thread;
		}
	}
	__lwt_schedule();	
}

lwt_t lwt_current(void){
	unsigned int offset;
	//unsigned int ret;
	//ret = (unsigned int) &offset - ((unsigned int) &offset % STACK_SIZE);
	//printf("%u to %u with %u\n", &offset, ret, (unsigned int) &offset % STACK_SIZE);
	return (unsigned int) &offset - ((unsigned int) &offset % STACK_SIZE);
}

int lwt_id(lwt_t thread){
	return *((unsigned int*) thread+1);
}

int lwt_info(lwt_info_t t){

}

void __lwt_schedule(void){
	assert(rq_head!=NULL);
	__lwt_dispatch(lwt_current(), rq_head);
}

void __lwt_dispatch(lwt_t current, lwt_t next){

	//printf("PUSH START!!!!!!!!!!");
	asm volatile ("pushl post_switch \n\t"
			"pushl %ebp \n\t"
			"pushl %eax \n\t"
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
			"popl %ebp \n"
			"post_switch: \n\t"
			"ret");

	asm volatile("movl %%esp, %0 \n\t"
			"mov %%edi, %1 \n\t"
			: "=r" (i), "=r" (j)
			: : );

	printf("stack pointer: %u, edi: %u\n", i, j);

	asm volatile("jmp %edi\n\t");

	//printf("\n");
	//printf("POP DONE!!!!!!!!!!");
}

void __lwt_trampoline(void){
	printf("boing\n");
	lwt_t current = lwt_current();

	printf("boinging %d on %u to %d\n", current->id, current, current->function);
	current->ret = current->function(current->param);

	printf("got %d\n", current->ret);
	lwt_die(current->ret);
	exit(0);
}
void * __lwt_stack_get(void){
	if(pool_head==NULL){
		printf("OUT OF FREE THREADS!\n");
		return NULL;
	}
	void* ret = pool_head;
	pool_head = pool_head->next;
	return ret;
}

void __lwt_stack_return(void *stk){
	lwt_t ret = stk;
	ret->next = pool_head;
	pool_head = ret;
}
void __lwt_stack_create(void){
	void* allthatmemory=aligned_alloc(STACK_SIZE, MAX_THREADS * STACK_SIZE);
	int i = 0;
	printf("allthat: %u\n", allthatmemory);
	for(i = 0; i<MAX_THREADS; i++){
		lwt_t current = allthatmemory + i*STACK_SIZE;
		if(i==0) current->next = NULL;
		else current->next = (int) current - STACK_SIZE;
		printf("thread at %u, next %u\n", current, current->next); 
	}
	pool_head = allthatmemory + (i-1)*STACK_SIZE;
	printf("head at %u\n", pool_head);
}

