#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#define STACK_SIZE (32 * 4096)
#define LWT_NULL NULL
#define MAX_THREADS 256
#define WAIT_FREE_RB_SIZE 512
#define MAX_UTIL_THDS 32

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
	GRP_BLK,
	SND_BLK,
	RCV_BLK,
	ZOMBIE,
	DEAD
} lwt_state_t;

typedef enum {
	LWT_JOIN,
	LWT_NOJOIN
} lwt_flags_t;

typedef void *(*lwt_fn_t)(void *);
typedef void *(*lwt_chan_fn_t)(lwt_chan_t);
typedef struct lwt_tcb* lwt_t;
typedef struct lwt_ktcb* lwt_kthd_t;
typedef struct lwt_channel* lwt_chan_t;
typedef struct lwt_chan_group* lwt_cgrp_t;
typedef struct lwt_msg* lwt_msg_t;

struct lwt_tcb {
	struct lwt_tcb *next, *prev, *joiner;
	unsigned int id;
	void *sp, *blk_data;
	void *param, *ret;
	lwt_state_t state;
	lwt_fn_t function;
	lwt_flags_t flags;
	volatile lwt_kthd_t owner;
};
struct lwt_channel {
	struct lwt_channel *next, *prev;
	lwt_t block_q_head, receiver;
	int snd_cnt, rcv_deref, event;
	void* mark;
	lwt_cgrp_t group;
	struct ring_buf* buf;
	volatile lwt_kthd_t owner;
};
struct lwt_chan_group {
	int chan_cnt;
	lwt_chan_t event_q_head;	
	volatile lwt_kthd_t owner;
};
struct ring_buf {
	volatile void** buffer;
	volatile unsigned int head, tail;
	unsigned int size, count;
};
struct lwt_ktcb {
	volatile struct lwt_ktcb* next, *prev, *block_q_head;
	volatile int asleep;
	pthread_t id;
	struct ring_buf* wf_rb;
	pthread_cond_t block_cv;
	pthread_mutex_t block_mutex;
};
struct lwt_kthd_init {
	pthread_t id;
	lwt_fn_t fn;
	lwt_chan_t c;
};
struct lwt_msg {
	lwt_chan_t c;
	void* data;
};
extern __thread lwt_kthd_t ktcb;
extern __thread lwt_t rq_head, idle;
extern __thread lwt_t pool_head, temp_pool;
extern __thread void* start;
extern __thread unsigned int global_id, n_chan, n_util_thds;

lwt_chan_t 		lwt_chan	(int sz);
void 	   		lwt_chan_deref	(lwt_chan_t c);
lwt_t			lwt_create_chan	(lwt_chan_fn_t fn, lwt_chan_t c, lwt_flags_t flags);
static inline void	lwt_chan_mark_set(lwt_chan_t c, void* mark);
static inline void*	lwt_chan_mark_get(lwt_chan_t c);
static inline int 	lwt_snd		(lwt_chan_t c, void* data);
static inline void* 	lwt_rcv		(lwt_chan_t c);
static inline int 	lwt_snd_chan	(lwt_chan_t c, lwt_chan_t sending);
static inline lwt_chan_t lwt_rcv_chan	(lwt_chan_t c);
static inline void	__lwt_snd_block (lwt_chan_t c, void* data);
static inline void*	__lwt_rcv_block (lwt_chan_t c);
static inline int	__lwt_snd_sync  (lwt_chan_t c, void* data);
static inline void*	__lwt_rcv_sync  (lwt_chan_t c);
static inline void	__lwt_snd_async (lwt_chan_t c, void* data);
static inline void*	__lwt_rcv_async (lwt_chan_t c);

lwt_cgrp_t 		lwt_cgrp	(void);
int 	   		lwt_cgrp_free	(lwt_cgrp_t grp);
int 	   		lwt_cgrp_add	(lwt_cgrp_t grp, lwt_chan_t c);
int 	   		lwt_cgrp_rem	(lwt_cgrp_t grp, lwt_chan_t c);
lwt_chan_t 		lwt_cgrp_wait	(lwt_cgrp_t grp);
static inline void	__lwt_cgrp_notify(lwt_chan_t c);
static inline void	__lwt_cgrp_denotify(lwt_chan_t c); 

int 			lwt_kthd_create (lwt_fn_t fn, lwt_chan_t c);
void			__lwt_kthd_init (struct lwt_kthd_init* new_init);
void			__lwt_setup	(void);
void			__lwt_idle	(void);
void			__lwt_kthd_sleep();
void			__lwt_kthd_wake (lwt_kthd_t wake_up);
int			__lwt_snd_msg   (lwt_chan_t c, void* data);
void			__lwt_rcv_msg	(lwt_msg_t msg);

void* 	   		lwt_join	(lwt_t thread);
void 	   		lwt_die		(void * ret);
lwt_t 	   		lwt_create	(lwt_fn_t fn, void *data, lwt_flags_t flags);
int 			lwt_info	(lwt_info_t t);
static inline lwt_t 	lwt_current	(void);
static inline int 	lwt_yield	(lwt_t thread);
static inline int 	lwt_id		(lwt_t thread);
static inline void 	__lwt_schedule	(void);
static inline void 	__lwt_trampoline(void);
static inline void 	__lwt_dispatch	(lwt_t next);
static inline lwt_t 	__lwt_stack_get	(void);
static inline void 	__lwt_stack_return(void *stk);
static inline void 	__lwt_stack_create(void);
static inline void 	__lwt_pool_merge(void);
static inline void* 	__lwt_block	(lwt_t thread, lwt_state_t state);
static inline void 	__lwt_unblock	(lwt_t thread, void* data);
static inline void 	__lwt_list_add	(void* prev, void* next, void* insert);
static inline void 	__lwt_list_rem	(void* prev, void* next);
static inline void	__lwt_enq_sleep (lwt_kthd_t kthd, lwt_kthd_t insert);
static inline void	__lwt_deq_wake  ();
static inline int	__lwt_rb_isfull (struct ring_buf* buf);
static inline int	__lwt_rb_isempty(struct ring_buf* buf);
static inline int 	__lwt_rb_add	(struct ring_buf* buf, void* value);
static inline void* 	__lwt_rb_rem	(struct ring_buf* buf);
static inline int	__lwt_rb_add_wf (struct ring_buf* buf, void* value);
static inline void* 	__lwt_rb_rem_wf (struct ring_buf* buf);
static inline struct ring_buf* __lwt_rb_create(int sz);
static inline void	__lwt_rb_destroy(struct ring_buf* buf);

static inline void lwt_chan_mark_set(lwt_chan_t c, void* mark){ c->mark = mark; }
static inline void* lwt_chan_mark_get(lwt_chan_t c){ return c->mark; }
static inline void __lwt_schedule(void){ __lwt_dispatch(rq_head); }
static inline int lwt_id(lwt_t thread){	return thread->id; }


static inline int lwt_snd_chan(lwt_chan_t c, lwt_chan_t sending){
	sending->snd_cnt++;
	return lwt_snd(c, sending);
}
static inline lwt_chan_t lwt_rcv_chan(lwt_chan_t c){ return lwt_rcv(c); }

static inline lwt_t lwt_current(void){
	int offset;
	return ((int) &offset & 0xfffe0000);
}
static inline int lwt_yield(lwt_t thread){
	rq_head = rq_head->next;
	if(__builtin_expect(thread, LWT_NULL)) goto direct;
	
	finish:
	__lwt_schedule();
	return 1;
	
	direct:
	if(thread == rq_head) goto finish;
	if(thread->state!=RUNNABLE || thread==lwt_current()) goto fail;	
	__lwt_list_rem(thread->prev, thread->next);
	__lwt_list_add(rq_head->prev, rq_head, thread); 	
	rq_head = thread;
	goto finish;
	
	fail:
	rq_head = rq_head->prev;
	return 0;	
}
static inline int lwt_snd(lwt_chan_t c, void* data){
	assert(data);	
	if(c->rcv_deref) return -1;
	if(c->owner != ktcb) return __lwt_snd_msg(c, data); 
	if(c->receiver->state == RCV_BLK) return __lwt_snd_sync(c, data);
	if(c->group) __lwt_cgrp_notify(c);
	c->event = 1;
	if(c->buf) __lwt_snd_async(c, data);
	else __lwt_snd_block(c, data);
	return 0;
}
static inline void* lwt_rcv(lwt_chan_t c){
	if(c->buf) return __lwt_rcv_async(c);
	if(c->block_q_head == c->block_q_head->next) return __lwt_rcv_block(c);
	return __lwt_rcv_sync(c);
}
static inline int __lwt_snd_sync(lwt_chan_t c, void* data){
	__lwt_unblock(c->receiver, data);
	return 0;
}
static inline void* __lwt_rcv_sync(lwt_chan_t c){
	lwt_t sender = c->block_q_head->next;
	__lwt_list_rem(sender->prev, sender->next);
	__lwt_unblock(sender, sender->blk_data);	
	if(c->group && c->block_q_head == c->block_q_head->next) __lwt_cgrp_denotify(c);
	return sender->blk_data;
}
static inline void __lwt_snd_async(lwt_chan_t c, void* data){
	if(__lwt_rb_isfull(c->buf)) __lwt_snd_block(c, data);
	else __lwt_rb_add(c->buf, data);
}
static inline void* __lwt_rcv_async(lwt_chan_t c){
	void* ret = __lwt_rb_rem(c->buf);
	if(!ret){
		if(c->block_q_head != c->block_q_head->next) return __lwt_rcv_sync(c);
		else return __lwt_rcv_block(c);
	}
	if(c->group && (c->block_q_head == c->block_q_head->next) && c->buf->count == 0) __lwt_cgrp_denotify(c);
	return ret;
}
static inline void __lwt_snd_block(lwt_chan_t c, void* data){
	lwt_t current = lwt_current();
	__lwt_list_rem(rq_head->prev, rq_head->next);
	rq_head = rq_head->next;
	__lwt_list_add(c->block_q_head->prev, c->block_q_head, current);	
	current->state = SND_BLK;
	current->blk_data = data;
	__lwt_schedule();
}
static inline void* __lwt_rcv_block(lwt_chan_t c){
	return __lwt_block(lwt_current(), RCV_BLK);
}
static inline void __lwt_cgrp_notify(lwt_chan_t c){
	if(c->receiver->state == GRP_BLK) __lwt_unblock(c->receiver, c);
	if(!c->event) __lwt_list_add(c->group->event_q_head->prev, c->group->event_q_head, c);
}	
static inline void __lwt_cgrp_denotify(lwt_chan_t c){
	__lwt_list_rem(c->prev, c->next);
	c->event = 0;
}
static inline void __lwt_trampoline(void){
	lwt_t current = lwt_current();
	current->ret = current->function(current->param);
	lwt_die(current->ret);
	assert(0);
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
			"and $0xfffe0000, %%eax \n\t"
			"movl %%esp, 0x10(%%eax) \n\t"
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
			:"memory","%esp", "%eax");
}
static inline lwt_t __lwt_stack_get(void){
	if(!pool_head){
		printf("OUT OF FREE THREADS!\n");
		return NULL;
	}
	lwt_t ret = pool_head;
	pool_head = pool_head->next;
	ret->sp = (int) ret + STACK_SIZE - 8;
	*((int*) ret->sp) = __lwt_trampoline;
	return ret;
}
static inline void __lwt_stack_return(void *stk){
	((lwt_t) stk)->next = pool_head;
	((lwt_t) stk)->prev = NULL;
	pool_head = stk;
}
static inline void __lwt_stack_create(void){
	void* allthatmemory;
	posix_memalign(&allthatmemory, STACK_SIZE, MAX_THREADS * STACK_SIZE);
	int i = 0;
	for(i = 0; i<MAX_THREADS; i++){
		lwt_t current = allthatmemory + i*STACK_SIZE;
		current->state = DEAD;
		if(i==0) current->next = NULL;
		else current->next = (int) current - STACK_SIZE;
	}
	pool_head = allthatmemory + (i-1)*STACK_SIZE;
	start = allthatmemory;
	temp_pool = NULL;
}
static inline void __lwt_pool_merge(void){
	while(temp_pool){
		lwt_t dead = temp_pool;
		temp_pool = temp_pool->next;
		__lwt_stack_return(dead);
	}
}
static inline void* __lwt_block(lwt_t thread, lwt_state_t state){
	thread->state = state;
	if(rq_head == rq_head->next && idle->state == BLOCKED) __lwt_unblock(idle, NULL);
	__lwt_list_rem(rq_head->prev, rq_head->next);
	lwt_yield(LWT_NULL);
	return thread->blk_data;
}
static inline void __lwt_unblock(lwt_t thread, void* data){
	__lwt_list_add(rq_head->prev, rq_head, thread);
	thread->state = RUNNABLE;
	thread->blk_data = data;
}
static inline void __lwt_list_add(void* prev, void* next, void* insert){
	((lwt_t) prev)->next = insert; 
	((lwt_t) insert)->prev = prev;
	((lwt_t) next)->prev = insert;
	((lwt_t) insert)->next = next;
}
static inline void __lwt_list_rem(void* prev, void* next){
	((lwt_t) prev)->next = next;
	((lwt_t) next)->prev = prev;
}
static inline void __lwt_enq_sleep(lwt_kthd_t kthd, lwt_kthd_t insert){
	pthread_mutex_lock(&(kthd->block_mutex));
	__lwt_list_add(kthd->block_q_head->prev, kthd->block_q_head, insert);
	pthread_mutex_unlock(&(kthd->block_mutex));
	__lwt_kthd_sleep();
}
static inline void __lwt_deq_wake(){
	pthread_mutex_lock(&(ktcb->block_mutex));
	while(ktcb->block_q_head != ktcb->block_q_head->next){
		__lwt_kthd_wake(ktcb->block_q_head->next);
		__lwt_list_rem(ktcb->block_q_head, ktcb->block_q_head->next->next);
	}
	pthread_mutex_unlock(&(ktcb->block_mutex));
}
static inline int __lwt_rb_isfull(struct ring_buf* buf){ return buf->count == buf->size; }
static inline int __lwt_rb_add(struct ring_buf* buf, void* value){
	if(__lwt_rb_isfull(buf)) return 0;
	buf->buffer[buf->tail] = value;
	buf->tail = (buf->tail+1)%(buf->size);
	buf->count++;
	return 1;
}
static inline int __lwt_rb_isempty(struct ring_buf* buf){ return buf->count == 0; }
static inline void* __lwt_rb_rem(struct ring_buf* buf){
	if(__lwt_rb_isempty(buf)) return 0;
	void* ret = buf->buffer[buf->head];
	buf->head = (buf->head+1)%(buf->size);
	buf->count--;
	return ret;
}
static inline struct ring_buf* __lwt_rb_create(int sz){
	struct ring_buf* new_buf = malloc(sizeof(struct ring_buf));
	new_buf->buffer = malloc(sizeof(void*)*sz);
	new_buf->head = new_buf->tail = new_buf->count = 0;
	new_buf->size = sz;
	return new_buf;
}
static inline void __lwt_rb_destroy(struct ring_buf* buf){
	free(buf->buffer);
	free(buf);
}
static inline int __lwt_rb_add_wf(struct ring_buf* buf, void* value){
	int new_tail = __sync_add_and_fetch(&(buf->tail), 1);
	if((new_tail-buf->head) >= buf->size){
		__sync_fetch_and_sub(&(buf->tail), 1);
		return 0;
	}
	buf->buffer[(new_tail-1)%(buf->size)] = value;
	return 1;
}
static inline void* __lwt_rb_rem_wf(struct ring_buf* buf){
	int old_head = __sync_fetch_and_add(&(buf->head), 1);	
	if((buf->tail)<=(old_head)){
		__sync_fetch_and_sub(&(buf->head), 1);
		return 0;
	}
	return buf->buffer[old_head%(buf->size)];
}
