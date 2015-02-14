#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define STACK_SIZE (8 * 4096)//Size of the stack
#define LWT_NULL NULL//This is for the scheduling
#define MAX_THREADS 256

typedef void *(*lwt_fn_t)(void *);

typedef struct thread {
	unsigned int id;
	struct thread *next, *prev, *joinlist;
	void *sp, *param, *ret, *joinret;
	lwt_fn_t function;
} *lwt_t;

typedef enum {
	LWT_INFO_NTHD_RUNNABLE,
	LWT_INFO_NTHD_BLOCKED,
	LWT_INFO_NTHD_ZOMBIES,
} lwt_info_t;

static volatile lwt_t rq_head, rq_tail;
static lwt_t pool_head;
static int NUM_ACTIVE=0, NUM_BLOCKED=0, NUM_ZOMB=0;
static unsigned int global_id = 1;

static inline void __lwt_schedule(void);
static volatile void __lwt_trampoline(void);
static inline void __lwt_dispatch(lwt_t next, lwt_t current);
static void * __lwt_stack_get(void);
static void __lwt_stack_return(void *stk);
static void __lwt_stack_create(void);

static lwt_t lwt_create(lwt_fn_t fn, void *data);
static inline lwt_t lwt_current(void);
static void * lwt_join(lwt_t thread);
static void lwt_die(void * ret);
static int lwt_yield(lwt_t thread);
static inline int lwt_id(lwt_t thread);
static inline int lwt_info(lwt_info_t t);
int main(void);

void __attribute__ ((constructor)) init(){
	__lwt_stack_create();
	lwt_t maint = lwt_create(main, NULL);
	__lwt_schedule();
}

static lwt_t lwt_create(lwt_fn_t fn, void *data){
	//printf("Start\n");	
	lwt_t lwt_thd = __lwt_stack_get();
	//printf("Thread Allocated!!!!!!! %u\n", lwt_thd);	
	lwt_thd->id = global_id;
	global_id++;
	//printf("ID Generated!!!!!!! %u\n", lwt_thd->id);
	void* sp = (int) lwt_thd + STACK_SIZE-4;
	//lwt_thd->sp = (unsigned int) lwt_thd->stack + STACK_SIZE;

	lwt_thd->param = data;
	lwt_thd->function = fn;
	lwt_thd->ret = -1;
	lwt_thd->joinlist = NULL;

	//printf("more data set, sp at %u\n", sp);
	*((int*) sp-1) = __lwt_trampoline;
	*((int*) sp-2) = sp;

	//printf("two things on the stack %u: %u at %u and %u at %u\n", sp, *((int*) sp-1), (int*)sp-1,  *((int*) sp-2), (int*)sp-2);
	lwt_thd->sp = sp-4;
	//printf("Stack Pointer set!!!!!!!\n");	
	//lwt_thd->ret = NULL;
	//printf("Value Set!!!!!!!\n");	

	lwt_thd->next = NULL;
	if(rq_head == NULL){
		rq_head = lwt_thd;
	}else{
		rq_tail->next = lwt_thd;
		lwt_thd->prev = rq_tail;
	}
	NUM_ACTIVE++;
	//printf("Thread %u, id %u, prev %u, next %u, rq_head %u, rq_tail %u\n", lwt_thd, lwt_thd->id, lwt_thd->prev, lwt_thd->next, rq_head, rq_tail);
	rq_tail = lwt_thd;

	/*lwt_t temp = rq_head;
	*while(temp!=NULL){
	*printf("rq %u %u\n", temp->id, temp);
	*temp = temp->next;
	*}*/
	//if(active == NULL) active = lwt_thd;
	return lwt_thd;
}

static void * lwt_join(lwt_t thread){
	lwt_t current = lwt_current();
	/*lwt_t temp = current;
	*while(temp!=NULL){
	*printf("rq %u %u\n", temp->id, temp);
	*temp = temp->next;
	*}*/
	//printf("%u, id %u joining %u\n", current, current->id, thread);
	//assert(current->next!=NULL);
	assert(rq_head==current);
	if(current == thread) return NULL;
	if(thread->ret!=-1){	
		//printf("Zombie put down, got %u!\n", thread->ret);
		NUM_ZOMB--;
		__lwt_stack_return(thread);
		return thread->ret;
	}
	rq_head = current->next;
	rq_head->prev = NULL;
	current->next = thread->joinlist;
	thread->joinlist = current;
	NUM_ACTIVE--;
	NUM_BLOCKED++;
	__lwt_schedule();
	//printf("back in action with %d\n", current->joinret);
	return current->joinret;
}

static void lwt_die(void * ret){
	lwt_t current = lwt_current();
	if(current->joinlist!=NULL){
		rq_tail->next = current->joinlist;
		current->joinlist->prev = rq_tail;
		/*while(current->joinlist->next!=NULL){
		*current->joinlist->joinret = ret;
		*current->joinlist->next->prev = current->joinlist;
		*current->joinlist=current->joinlist->next;
		*}*/
		current->joinlist->joinret = ret;
		rq_tail = current->joinlist;
		rq_tail->next = NULL;
		/*lwt_t temp = rq_head;
		*while(temp!=NULL){
		*printf("rq %u %u\n", temp->id, temp);
		*temp = temp->next;
		*}*/
		NUM_BLOCKED--;
		NUM_ACTIVE++;
		//printf("dying\n");

	}else{
		NUM_ZOMB++;
		// printf("brainsssss\n");
	}
	if(rq_head->next == NULL){
		//printf("All done\n");
		exit(0);
	}
	assert(rq_head == current);
	rq_head = rq_head->next;
	rq_head->prev = NULL;
	NUM_ACTIVE--;
	if(current->joinlist != NULL) __lwt_stack_return(current);
	__lwt_schedule();
}

static int lwt_yield(lwt_t thread){
	lwt_t current = lwt_current();
	/*printf("Current: %u, next %u\n", current, thread);
	*wt_t temp = rq_head;
	*hile(temp!=NULL){
	*rintf("rq %u %u\n", temp->id, temp);
	*temp = temp->next;
	*}*/
	//assert(rq_head == current);

	if(thread == current || current->next == NULL) return 1;
	/*if(current->next!=NULL){
	*q_head = current->next;
	*current->next=NULL;
	*}
	*if(rq_tail!=current){
	*rq_tail->next = current;
	*current->prev = rq_tail;
	*rq_tail = current;
	*}*/
	rq_head = current->next;
	rq_head->prev = NULL;
	rq_tail->next = current;
	current->prev = rq_tail;
	rq_tail = current;
	current->next = NULL;

	if(thread != LWT_NULL && thread!= rq_head){
		if(thread->id < 1 || thread->id > global_id) return 0;

		//if(thread!=rq_head){
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
		//}
	}
	/*******************************/
	//printf("I yield, next is %u!\n", rq_head);
	__lwt_schedule();	
}

static inline lwt_t lwt_current(void){
	unsigned int offset;
	//unsigned int ret;
	//ret = (unsigned int) &offset - ((unsigned int) &offset % STACK_SIZE);
	//printf("%u to %u with %u\n", &offset, ret, (unsigned int) &offset % STACK_SIZE);
	return (unsigned int) &offset - ((unsigned int) &offset % STACK_SIZE);
	//return active;
}

static inline int lwt_id(lwt_t thread){
	return *((unsigned int*) thread);
}

static inline int lwt_info(lwt_info_t t){
	if(t == LWT_INFO_NTHD_RUNNABLE) return NUM_ACTIVE;
	if(t == LWT_INFO_NTHD_BLOCKED) return NUM_BLOCKED;
	if(t == LWT_INFO_NTHD_ZOMBIES) return NUM_ZOMB;
	return 0; 
}

static inline void __lwt_schedule(void){
	assert(rq_head != NULL);
	__lwt_dispatch(lwt_current(), rq_head);
}

static volatile inline void __lwt_dispatch(lwt_t current, lwt_t next){

	//int i=0, j=0;
	//int* dumb;	
	/*asm volatile("movl $ps_%=, %[out]\n\t"
	*:[out]"=r" (i) : :); */

	/***********************************/
	//printf("dispatching from %u to %u, sp: %u\n", current, next, next->sp);
	//printf("popping to %u, pushing %u\n", *((int*) next->sp),i);
	//active = next;

	asm volatile (	"pushl %ebp \n\t"
			"pushl %eax \n\t"
			"pushl %ebx \n\t"
			"pushl %ecx \n\t"
			"pushl %edx \n\t"
			"pushl %esi \n\t"
			"pushl %edi \n\t"
			"push $1f \n\t" );

	asm volatile (	"movl %%esp, %0 \n\t"
			"movl %1, %%esp \n\t"
			:"=g" (current->sp)
			:"g" (next->sp)
			:"memory", "%esp");

	/*printf("old stack: \n");
	*for(i = 0; i<=8; i++)
	*printf("stack %u: %u: %u\n", current->sp, (int*) current->sp+i, *((int*) current->sp+i));	
	*printf("new stack: \n");
	*for(i = 0; i<=8; i++)
	*printf("stack %u: %u: %u\n", next->sp, (int*) next->sp+i, *((int*) next->sp+i));	
	**/

	asm volatile ("popl %edi \n\t"
			"jmp %edi\n\n"
			"1: \n\t"
			"popl %edi \n\t"
			"popl %esi \n\t"
			"popl %edx \n\t"
			"popl %ecx \n\t"
			"popl %ebx \n\t"
			"popl %eax \n\t"
			"popl %ebp \n\t");

	//should stop here
	/*asm volatile("movl %%esp, %0 \n\t"
	*"mov %%edi, %1 \n\t"
	*: "=r" (i), "=r" (j)
	*: : );
	*printf("stack pointer: %u, edi: %d\n", i, j);*/


	//assert(0);
	//asm volatile("jmp %edi\n\t");

	//printf("\n");
	//printf("POP DONE!!!!!!!!!!");
}

static volatile void __lwt_trampoline(void){
	//printf("boing\n");
	lwt_t current = lwt_current();

	/**********************************/
	//printf("boinging %d on %u to %d\n", current->id, current, current->function);
	current->ret = current->function(current->param);

	//printf("got %d\n", current->ret);
	lwt_die(current->ret);
	assert(0);
}
static void * __lwt_stack_get(void){
	if(pool_head==NULL){
		printf("OUT OF FREE THREADS!\n");
		return NULL;
	}
	void* ret = pool_head;
	pool_head = pool_head->next;
	return ret;
}

static void __lwt_stack_return(void *stk){
	lwt_t ret = stk;
	ret->next = pool_head;
	ret->prev = NULL;
	pool_head = ret;
}
static void __lwt_stack_create(void){
	void* allthatmemory=aligned_alloc(STACK_SIZE, MAX_THREADS * STACK_SIZE);
	int i = 0;
	//printf("allthat: %u\n", allthatmemory);
	for(i = 0; i<MAX_THREADS; i++){
		lwt_t current = allthatmemory + i*STACK_SIZE;
		if(i==0) current->next = NULL;
		else current->next = (int) current - STACK_SIZE;
		//printf("thread at %u, next %u\n", current, current->next); 
	}
	pool_head = allthatmemory + (i-1)*STACK_SIZE;
	//printf("head at %u\n", pool_head);
}

