#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define STACK_SIZE (8 * 4096)
#define LWT_NULL NULL
#define MAX_THREADS 256

typedef enum {
	LWT_INFO_NTHD_RUNNABLE,
	LWT_INFO_NTHD_BLOCKED,
	LWT_INFO_NTHD_ZOMBIES,
	LWT_INFO_NCHAN,
	LWT_INFO_NSNDING,
	LWT_INFO_NRCVING
} lwt_info_t;

typedef enum {
	RUNNABLE,
	BLOCKED,
	ZOMBIE,
	DEAD
} lwt_state_t;

typedef void *(*lwt_fn_t)(void *);

typedef struct lwt_tcb {
	unsigned int id;
	volatile void *sp, *joinret;
	struct lwt_tcb *next, *prev, *joiner, *snd_next, *snd_prev;
	void *param, *ret, *snd_data;
	lwt_state_t state;
	lwt_fn_t function;
} *lwt_t;

typedef struct lwt_channel {
	int snd_cnt;
	lwt_t senders_head, senders_tail;
	lwt_t block_q_head, block_q_tail;
	void* rcv_data;
	int rcv_cnt;
	lwt_t receiver;
} *lwt_chan_t;

typedef void *(*lwt_chan_fn_t)(lwt_chan_t);

extern volatile lwt_t rq_head, rq_tail;
extern lwt_t pool_head;
extern void* start;
extern unsigned int global_id;

void * lwt_join(lwt_t thread);
void lwt_die(void * ret);
lwt_t lwt_create(lwt_fn_t fn, void *data);

lwt_chan_t lwt_chan(int sz);
void lwt_chan_deref(lwt_chan_t c);
int lwt_snd(lwt_chan_t c, void* data);
void* lwt_rcv(lwt_chan_t c);
int lwt_snd_chan(lwt_chan_t c, lwt_chan_t sending);
lwt_chan_t lwt_rcv_chan(lwt_chan_t c);
lwt_t lwt_create_chan(lwt_chan_fn_t fn, lwt_chan_t c);

static inline lwt_t lwt_current(void);
static inline int lwt_yield(lwt_t thread);
static inline int lwt_id(lwt_t thread);
static inline int lwt_info(lwt_info_t t);

static inline void __lwt_schedule(void);
static inline void __lwt_trampoline(void);
static inline void __lwt_dispatch(lwt_t next);
static inline void * __lwt_stack_get(void);
static inline void __lwt_stack_return(void *stk);
static inline void __lwt_stack_create(void);

static inline void __lwt_enqueue(lwt_t *head, lwt_t *tail, lwt_t insert);
static inline lwt_t __lwt_dequeue(lwt_t *head, lwt_t *tail);

static inline int lwt_yield(lwt_t thread){
	rq_tail = rq_head;	
	rq_head = rq_head->next;

	if(thread==LWT_NULL || thread==rq_head) goto finish;	
	if(thread->state!=RUNNABLE || thread==lwt_current()) goto fail;

	thread->prev->next = thread->next;
	thread->next->prev = thread->prev;
	
	rq_tail->next = thread;
	thread->prev = rq_tail;
	thread->next = rq_head;
	rq_head->prev = thread;
	rq_head = thread;
	
	finish:
	__lwt_schedule();
	return 1;

	fail:
	rq_head = rq_tail;
	rq_tail = rq_tail->prev;
	return 0;	
}

static inline lwt_t lwt_current(void){
	int offset;
	return ((int) &offset & 0xffff8000);
}

static inline int lwt_id(lwt_t thread){
	return *((unsigned int*) thread);
}

static inline int lwt_info(lwt_info_t t){
	int active=0, blocked=0, dead=0, i=0;

	for(i = 0; i<MAX_THREADS; i++){
		if(((lwt_t) (start+i*STACK_SIZE))->state == RUNNABLE) active++;
		if(((lwt_t) (start+i*STACK_SIZE))->state == BLOCKED) blocked++;
		if(((lwt_t) (start+i*STACK_SIZE))->state == ZOMBIE) dead++;
	}
	if(t == LWT_INFO_NTHD_RUNNABLE){
		return active;
	}
	if(t == LWT_INFO_NTHD_BLOCKED){
		return blocked;
	}
	if(t == LWT_INFO_NTHD_ZOMBIES){
		return dead;
	}
	return 0; 
}

static inline void __lwt_schedule(void){
	assert(rq_head!=NULL);
	__lwt_dispatch(rq_head);
}

static inline void __lwt_dispatch(lwt_t next){
 
	asm volatile (	"pushl %%eax \n\t"
			"pushl %%ebx \n\t"
			"pushl %%ecx \n\t"
			"pushl %%edx \n\t"
			"pushl %%esi \n\t"
			"pushl %%edi \n\t"
			"pushl %%ebp \n\t"
			"push $1f \n\t" 
			"movl %%esp, %%eax \n\t"
			"movl %%eax, %%edx \n\t"
			"and $0xffff8000, %%edx \n\t"
			"movl %%eax, 0x4(%%edx) \n\t"
			"movl %0, %%esp \n\t"
			"ret \n\t"
			"1: \n\t"
			"popl %%ebp \n\t"
			"popl %%edi \n\t"
			"popl %%esi \n\t"
			"popl %%edx \n\t"
			"popl %%ecx \n\t"
			"popl %%ebx \n\t"
			"popl %%eax \n\t"
			:	
			:"g" (next->sp)
			:"memory", "%esp", "%eax", "%ebx", "%ecx", "%edx", "%esi", "%esi", "%ebp");
}

static inline void __lwt_trampoline(void){
	volatile lwt_t current = lwt_current();

	current->ret = current->function(current->param);
	lwt_die(current->ret);
	assert(0);
}
static inline void * __lwt_stack_get(void){
	if(pool_head==NULL){
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
static inline void __lwt_stack_create(void){
	void* allthatmemory=aligned_alloc(STACK_SIZE, MAX_THREADS * STACK_SIZE);
	int i = 0;
	for(i = 0; i<MAX_THREADS; i++){
		lwt_t current = allthatmemory + i*STACK_SIZE;
		current->state = DEAD;
		if(i==0) current->next = NULL;
		else current->next = (int) current - STACK_SIZE;
	}
	pool_head = allthatmemory + (i-1)*STACK_SIZE;
	start = allthatmemory;
}
static inline void __lwt_enqueue(lwt_t* head, lwt_t *tail, lwt_t insert){
	if(*head==NULL){
		*head = insert;
		insert->prev = insert;
		*tail = insert;
	} else insert->prev = (*head)->prev;

	insert->next = *head;
	(*tail)->next = insert;
	(*head)->prev = insert;
	*tail = insert;
}
static inline lwt_t __lwt_dequeue(lwt_t *head, lwt_t *tail){
	lwt_t ret = *head;
	*head = (*head)->next;
	(*head)->prev = ret->prev;
	(*tail)->next = *head;
	return ret;
} 
