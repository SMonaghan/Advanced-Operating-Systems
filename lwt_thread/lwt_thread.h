#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define STACK_SIZE (8 * 4096)//Size of the stack
#define LWT_NULL (-1)//This is for the scheduling
#define MAX_THREADS 256

typedef void *(*lwt_fn_t)(void *);

typedef struct thread {
	unsigned int id;
	struct thread *next, *prev, *joinlist;
	volatile void *sp, *joinret;
	void *param, *ret;
	lwt_fn_t function;
} *lwt_t;

typedef enum {
	LWT_INFO_NTHD_RUNNABLE,
	LWT_INFO_NTHD_BLOCKED,
	LWT_INFO_NTHD_ZOMBIES,
} lwt_info_t;

static volatile lwt_t rq_head, rq_tail;
static lwt_t pool_head;
static unsigned int global_id = 1;

static inline void __lwt_schedule(void);
static volatile void __lwt_trampoline(void);
static inline void __lwt_dispatch(lwt_t next);
static inline void * __lwt_stack_get(void);
static inline void __lwt_stack_return(void *stk);
static void __lwt_stack_create(void);

static lwt_t lwt_create(lwt_fn_t fn, void *data);
static inline lwt_t lwt_current(void);
static void * lwt_join(lwt_t thread);
static void lwt_die(void * ret);
static inline int lwt_yield(lwt_t thread);
static inline int lwt_id(lwt_t thread);
static inline int lwt_info(lwt_info_t t);
int main(void);

void __attribute__ ((constructor)) init(){
	__lwt_stack_create();
	lwt_t maint = lwt_create(main, NULL);
	__lwt_schedule();
}

static lwt_t lwt_create(lwt_fn_t fn, void *data){
	lwt_t lwt_thd = __lwt_stack_get();
	lwt_thd->id = global_id;
	global_id++;
	void* sp = (int) lwt_thd + STACK_SIZE-4;
	
	lwt_thd->param = data;
	lwt_thd->function = fn;
	lwt_thd->ret = LWT_NULL;
	lwt_thd->joinlist = NULL;

	*((int*) sp-1) = __lwt_trampoline;
	*((int*) sp-2) = sp;

	lwt_thd->sp = sp-4;

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

static void * lwt_join(lwt_t thread){
	lwt_t current = lwt_current();
	assert(rq_head==current);
	if(current == thread) return NULL;
	
	if(thread->ret!=LWT_NULL){	
		__lwt_stack_return(thread);
		return thread->ret;
	}
	rq_head = current->next;
	rq_head->prev = NULL;
	current->next = thread->joinlist;
	thread->joinlist = current;
	__lwt_schedule();
	return current->joinret;
}

static void lwt_die(void * ret){
	lwt_t current = lwt_current();
	if(current->joinlist!=NULL){
		rq_tail->next = current->joinlist;
		current->joinlist->prev = rq_tail;
		current->joinlist->joinret = ret;
		rq_tail = current->joinlist;
		rq_tail->next = NULL;

	}
	if(rq_head->next == NULL){
		exit(0);
	}
	assert(rq_head==current);
	rq_head = rq_head->next;
	rq_head->prev = NULL;
	if(current->joinlist!=NULL) __lwt_stack_return(current);
	__lwt_schedule();
}

static inline int lwt_yield(lwt_t thread){
	lwt_t current = lwt_current();
	assert(current == rq_head);
	if(thread == current || current->next == NULL)  return 1; 

	rq_head = current->next;
	rq_head->prev = NULL;
	rq_tail->next = current;
	current->prev = rq_tail;
	rq_tail = current;
	current->next = NULL;

	if(thread != LWT_NULL && thread != rq_head){
		if(thread->ret != LWT_NULL) return 0;
		
		if(thread != rq_tail){
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
	__lwt_schedule();	
}

static inline lwt_t lwt_current(void){
	unsigned int offset;
	return ((int) &offset & 0xffff8000);
}

static inline int lwt_id(lwt_t thread){
	return *((unsigned int*) thread);
}

static inline int lwt_info(lwt_info_t t){
	
	lwt_t temp = rq_head;
	int active = 0, blocked = 0, dead = 0;

	while(temp != NULL){
		if(temp->joinlist != NULL) blocked++;
		active++;
		temp = temp->next;
	}
	temp = pool_head;
	while(temp != NULL){
		dead++;
		temp = temp->next;
	}

	if(t == LWT_INFO_NTHD_RUNNABLE) return active;
	if(t == LWT_INFO_NTHD_BLOCKED) return blocked;
	if(t == LWT_INFO_NTHD_ZOMBIES) return MAX_THREADS-dead-blocked-active;
	return 0; 
}

static inline void __lwt_schedule(void){
	assert(rq_head!=NULL);
	__lwt_dispatch(rq_head);
}

static inline void __lwt_dispatch(lwt_t next){

	asm volatile (	"pushl %ebp \n\t"
			"pushl %eax \n\t"
			"pushl %ebx \n\t"
			"pushl %ecx \n\t"
			"pushl %edx \n\t"
			"pushl %esi \n\t"
			"pushl %edi \n\t"
			"push $1f \n\t" );

	asm volatile(	"movl %%esp, %%eax \n\t"
			"movl %%eax, %%edx \n\t"
			"and $0xffff8000, %%edx \n\t"
			"movl %%eax, 0x10(%%edx) \n\t"
			"movl %0, %%esp \n\t"
			:	
			:"g" (next->sp)
			:"memory", "%esp", "%eax", "%edx");

	/*asm volatile( 	"movl %%esp, %0 \n\t"
			"movl %1, %%esp \n\t"
			: "=g" (current->sp)
			: "g" (next->sp)
			: "memory", "%esp");*/
	
	asm volatile (	"ret \n\n"
			"1: \n\t"
			"popl %edi \n\t"
			"popl %esi \n\t"
			"popl %edx \n\t"
			"popl %ecx \n\t"
			"popl %ebx \n\t"
			"popl %eax \n\t"
			"popl %ebp \n\t");
}

static volatile void __lwt_trampoline(void){
	volatile lwt_t current = lwt_current();

	current->ret = current->function(current->param);
	lwt_die(current->ret);
	assert(0);
}
static inline void * __lwt_stack_get(void){
	if(pool_head == NULL){
		printf("OUT OF FREE THREADS!\n");
		return NULL;
	}
	void* ret = pool_head;
	pool_head = pool_head->next;
	return ret;
}

static inline void __lwt_stack_return(void *stk){
	lwt_t ret = stk;
	ret->next = pool_head;
	ret->prev = NULL;
	pool_head = ret;
}
static void __lwt_stack_create(void){
	void* allthatmemory = aligned_alloc(STACK_SIZE, MAX_THREADS * STACK_SIZE);
	int i = 0;
	for(i = 0; i < MAX_THREADS; i++){
		lwt_t current = allthatmemory + i*STACK_SIZE;
		if(i == 0) current->next = NULL;
		else current->next = (int) current - STACK_SIZE;
	}
	pool_head = allthatmemory + (i-1)*STACK_SIZE;
}

