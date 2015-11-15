#include "lwt.h"

__thread lwt_kthd_t ktcb;
__thread lwt_t rq_head, idle;
__thread lwt_t pool_head, temp_pool;
__thread void* start;
__thread unsigned int global_id, n_chan, n_util_thds;

int main(void);

void __attribute__ ((constructor)) __lwt_init(){
	__lwt_setup();
	ktcb->id = NULL;
	__lwt_schedule();
}
void __lwt_setup(){
	__lwt_stack_create();
	global_id = 1;
	n_chan = 0;
	lwt_t maint = __lwt_stack_get();
	maint->id = global_id;
	global_id++;
	maint->function = main;
	maint->flags = LWT_NOJOIN;
	
	ktcb = malloc(sizeof(struct lwt_ktcb));
	ktcb->wf_rb = __lwt_rb_create(WAIT_FREE_RB_SIZE);
	pthread_cond_init(&(ktcb->block_cv), NULL);
	pthread_mutex_init(&(ktcb->block_mutex), NULL);
	maint->owner = ktcb;
	ktcb->block_q_head = malloc(sizeof(struct lwt_ktcb));
	ktcb->block_q_head->next = ktcb->block_q_head->prev = ktcb->block_q_head;

	rq_head = maint->next = maint->prev = maint;
	maint->state = RUNNABLE;
	idle = lwt_create(__lwt_idle, NULL, LWT_NOJOIN);
	idle->state = BLOCKED;
	__lwt_list_rem(idle->prev, idle->next);
}
int lwt_kthd_create(lwt_fn_t fn, lwt_chan_t c){
	struct lwt_kthd_init* new_init = malloc(sizeof(struct lwt_kthd_init));
	new_init->fn = fn;
	new_init->c = c;
	
	int ret = pthread_create(&(new_init->id), NULL, __lwt_kthd_init, new_init);
	pthread_detach(new_init->id);
	if(ret!=0) return -1;
	return 0;
}
void __lwt_kthd_init(struct lwt_kthd_init* new_init){
	__lwt_setup();
	ktcb->id = new_init->id;
	rq_head->function = new_init->fn;
	rq_head->param = new_init->c;
	new_init->c->snd_cnt++;
	free(new_init);
	__lwt_schedule();
}
void __lwt_idle(){
	lwt_msg_t msg;
	while(1){
		msg = __lwt_rb_rem_wf(ktcb->wf_rb);		
		while(msg){	
			msg_process:
			if(msg->c->receiver->state != RCV_BLK && msg->c->buf && !__lwt_rb_isfull(msg->c->buf)){
				lwt_create(__lwt_rcv_msg, msg, LWT_NOJOIN);
				n_util_thds++;
				if(n_util_thds >= MAX_UTIL_THDS) lwt_yield(LWT_NULL);
			}
			else lwt_snd(msg->c, msg->data);
			msg = __lwt_rb_rem_wf(ktcb->wf_rb);
		}
		__lwt_deq_wake();
		if(rq_head->next == rq_head){
			//printf("%d idle sleep, head: %d, tail: %d\n", ktcb, ktcb->wf_rb->head, ktcb->wf_rb->tail);
			msg = __lwt_rb_rem_wf(ktcb->wf_rb);
			if(!msg) __lwt_kthd_sleep();
			else goto msg_process;
		}
		if(rq_head != rq_head->next){
			//printf("idle sleeping\n");
			__lwt_block(lwt_current(), BLOCKED);	
		}
	}
}
void __lwt_kthd_sleep(){
	pthread_mutex_lock(&(ktcb->block_mutex));
	ktcb->asleep = 1;
	while(ktcb->asleep) pthread_cond_wait(&(ktcb->block_cv), &(ktcb->block_mutex));
	pthread_mutex_unlock(&(ktcb->block_mutex));
	//printf("%d awake!\n", ktcb);
}
void __lwt_kthd_wake(lwt_kthd_t wake_up){
	pthread_mutex_lock(&(wake_up->block_mutex));
	wake_up->asleep = 0;
	pthread_cond_signal(&(wake_up->block_cv));
	pthread_mutex_unlock(&(wake_up->block_mutex));
}
int __lwt_snd_msg(lwt_chan_t c, void* data){
	lwt_msg_t new_msg = malloc(sizeof(struct lwt_msg));
	new_msg->c = c;
	new_msg->data = data;
	
	while(!__lwt_rb_add_wf(c->owner->wf_rb, new_msg)){
		//printf("buffer full, %d waiting\n", ktcb);
		__lwt_enq_sleep(c->owner, ktcb);
		//__lwt_kthd_sleep();
	}
	//printf("%d sent msg, head: %d, tail: %d\n", ktcb, c->owner->wf_rb->head, c->owner->wf_rb->tail);
	if(c->owner->asleep){
		__lwt_kthd_wake(c->owner);
	}
	return 0;
}
void __lwt_rcv_msg(lwt_msg_t msg){
	lwt_snd(msg->c, msg->data);
	free(msg);
	n_util_thds--;
}
lwt_t lwt_create(lwt_fn_t fn, void *data, lwt_flags_t flags){
	lwt_t lwt_thd = __lwt_stack_get();
	if(temp_pool) __lwt_pool_merge();
	lwt_thd->id = global_id;
	global_id++;
	
	lwt_thd->param = data;
	lwt_thd->function = fn;
	lwt_thd->ret = LWT_NULL;
	lwt_thd->joiner = LWT_NULL;
	lwt_thd->flags = flags;
	lwt_thd->owner = ktcb;
	
	__lwt_list_add(rq_head->prev, rq_head, lwt_thd);
	lwt_thd->state = RUNNABLE;
	return lwt_thd;
}
lwt_t lwt_create_chan(lwt_chan_fn_t fn, lwt_chan_t c, lwt_flags_t flags){
	lwt_t lwt = lwt_create(fn, c, flags);
	c->snd_cnt++;
	return lwt;
}
void * lwt_join(lwt_t thread){
	lwt_t current = lwt_current();
	void* ret;
	if(current == thread) return NULL;
	if(thread->state == ZOMBIE){
		ret = thread->ret;
		thread->state = DEAD;
	} else {
		thread->joiner = current;
		ret = __lwt_block(current, BLOCKED);
	}
	__lwt_stack_return(thread);
	return ret;
}
void lwt_die(void * ret){
	lwt_t current = lwt_current();
	current->state = DEAD;
	if(current->joiner == LWT_NULL) current->state = ZOMBIE;
	else __lwt_unblock(current->joiner, ret);	
	if(current->id == 1){
		if(ktcb->id == NULL) exit(0);
		else pthread_exit(ret);
	}
	if(current->next == current && idle->state == BLOCKED) __lwt_unblock(idle, NULL);
	__lwt_list_rem(current->prev, current->next);
	rq_head = current->next;
	if(current->flags == LWT_NOJOIN){
		current->next = temp_pool;
		temp_pool = current;
	}
	__lwt_schedule();
}
int lwt_info(lwt_info_t t){
	int active=0, blocked=0, zombies=0, snd=0, rcv=0, i=0;

	for(i = 0; i<MAX_THREADS; i++){
		if(((lwt_t) (start+i*STACK_SIZE))->state == RUNNABLE) active++;
		if(((lwt_t) (start+i*STACK_SIZE))->state == BLOCKED) blocked++;
		if(((lwt_t) (start+i*STACK_SIZE))->state == ZOMBIE) zombies++;
		if(((lwt_t) (start+i*STACK_SIZE))->state == SND_BLK) snd++;
		if(((lwt_t) (start+i*STACK_SIZE))->state == RCV_BLK) rcv++;
	}
	if(t == LWT_INFO_NTHD_RUNNABLE) return active;
	if(t == LWT_INFO_NTHD_BLOCKED)	return blocked;
	if(t == LWT_INFO_NTHD_ZOMBIES)	return zombies;
	if(t == LWT_INFO_NSNDING) 	return snd;
	if(t == LWT_INFO_NRCVING)	return rcv;
	if(t == LWT_INFO_NCHAN)		return n_chan;
	return 0; 
}
lwt_chan_t lwt_chan(int sz){
	lwt_chan_t chan = malloc(sizeof(struct lwt_channel));
	chan->next = chan->prev = chan;
	chan->snd_cnt = chan->rcv_deref = chan->group = chan->mark = chan->buf = 0;;
	chan->block_q_head = malloc(sizeof(struct lwt_tcb));
	chan->block_q_head->next = chan->block_q_head->prev = chan->block_q_head;
	chan->receiver = lwt_current();
	chan->owner = ktcb;
	n_chan++;
	if(sz > 0) chan->buf = __lwt_rb_create(sz);
	return chan;	
}
void lwt_chan_deref(lwt_chan_t c){
	if(c->receiver == lwt_current()) c->rcv_deref = 1;
	else c->snd_cnt--;
	if(c->snd_cnt > 0 || !c->rcv_deref) return;
	if(c->buf) __lwt_rb_destroy(c->buf);
	free(c);
	n_chan--;
}
lwt_cgrp_t lwt_cgrp(){
	lwt_cgrp_t grp = malloc(sizeof(struct lwt_chan_group));
	grp->chan_cnt = 0;
	grp->event_q_head = malloc(sizeof(struct lwt_channel));
	grp->event_q_head->next = grp->event_q_head->prev = grp->event_q_head;
	grp->owner = ktcb;
	return grp;
}
int lwt_cgrp_free(lwt_cgrp_t grp){
	if(grp->chan_cnt > 0) return -1;
	free(grp);
	return 0;
}
int lwt_cgrp_add(lwt_cgrp_t grp, lwt_chan_t c){
	if(c->group) return -1;
	if(c->event) __lwt_list_add(grp->event_q_head->prev, grp->event_q_head, c);
	grp->chan_cnt++;
	c->group = grp;
	return 0;
}
int lwt_cgrp_rem(lwt_cgrp_t grp, lwt_chan_t c){
	if(c->group != grp) return -1;
	if(c->event) return 1;
	grp->chan_cnt--;
	c->group = NULL;
	return 0;
}
lwt_chan_t lwt_cgrp_wait (lwt_cgrp_t grp){
	if(grp->event_q_head == grp->event_q_head->next) __lwt_block(lwt_current(), GRP_BLK);
	return grp->event_q_head->next;	
}
