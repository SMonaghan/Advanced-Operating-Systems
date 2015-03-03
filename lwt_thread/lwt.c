#include "lwt.h"

volatile lwt_t rq_head, rq_tail;
lwt_t pool_head;
void* start;
unsigned int global_id;

int main(void);

void __attribute__ ((constructor)) lwt_init(){
	__lwt_stack_create();
	global_id = 1;
	lwt_t maint = lwt_create(main, NULL);
	__lwt_schedule();
}

lwt_t lwt_create(lwt_fn_t fn, void *data){
	lwt_t lwt_thd = __lwt_stack_get();
	lwt_thd->id = global_id;
	global_id++;
	void* sp = (int) lwt_thd + STACK_SIZE-4;
	
	lwt_thd->param = data;
	lwt_thd->function = fn;
	lwt_thd->ret = LWT_NULL;
	lwt_thd->joiner = LWT_NULL;
	
	*((int*) sp-1) = __lwt_trampoline;
	*((int*) sp-2) = 0;

	lwt_thd->sp = sp-4;
	__lwt_enqueue(&rq_head, &rq_tail, lwt_thd);

	lwt_thd->state = RUNNABLE;
	return lwt_thd;
}

void * lwt_join(lwt_t thread){
	lwt_t current = lwt_current();
	
	if(current == thread) return NULL;
	if(thread->state == ZOMBIE) goto zombie;	
	
	assert(__lwt_dequeue(&rq_head, &rq_tail)==current);
	thread->joiner = current;
	current->state = BLOCKED;
	__lwt_schedule();
	end:
	__lwt_stack_return(thread);
	return current->joinret;

	zombie:
	current->joinret = thread->ret;
	thread->state = DEAD;
	goto end;
}

void lwt_die(void * ret){
	lwt_t current = lwt_current();
	if(current->joiner == LWT_NULL) goto zombie;

	current->joiner->joinret = ret;
	__lwt_enqueue(&rq_head, &rq_tail, current->joiner);	
	current->joiner->state = RUNNABLE;
	current->state = DEAD;
	end:
	if(rq_head->next == rq_head) exit(0);
	__lwt_dequeue(&rq_head, &rq_tail);
	__lwt_schedule();
	
	zombie:
	current->state = ZOMBIE;
	goto end;
}

lwt_chan_t lwt_chan(int sz){
	return NULL;
}

void lwt_chan_deref(lwt_chan_t c){
	return;
}

int lwt_snd(lwt_chan_t c, void* data){
	assert(data!=NULL);	
	if(c->rcv_cnt == -1) return -1;

	if(c->rcv_cnt == 1){
		c->rcv_data = data;
		c->rcv_cnt = 0;
		__lwt_enqueue(&rq_head, &rq_tail, c->receiver);
		c->receiver->state = RUNNABLE;
		//yield?
	}else if(c->rcv_cnt == 0){
		lwt_t current = lwt_current();
		current->snd_data = data;
		__lwt_dequeue(&rq_head, &rq_tail);
		__lwt_enqueue(&(c->block_q_head), &(c->block_q_tail), current);
		current->state = BLOCKED;
		__lwt_schedule(); //or directed yield?
	}
	return 0;
}

void* lwt_rcv(lwt_chan_t c){
	if(c->snd_cnt==0) return NULL;
	if(c->block_q_head != NULL){ //assume this is how it works
		lwt_t sender = __lwt_dequeue(&(c->block_q_head), &(c->block_q_tail));
		__lwt_enqueue(&rq_head, &rq_tail, sender);
		sender->state = RUNNABLE;
		return sender->snd_data;
	}else{
		lwt_t current =	__lwt_dequeue(&rq_head, &rq_tail);
		current->state = BLOCKED;
		c->rcv_cnt = 1;
		__lwt_schedule();
		return c->rcv_data;
	}
}

int lwt_snd_chan(lwt_chan_t c, lwt_chan_t sending){
	sending->snd_cnt++;
	return lwt_snd(c, sending);
}

lwt_chan_t lwt_rcv_chan(lwt_chan_t c){
	lwt_chan_t rcving = lwt_rcv(c);
	lwt_t current = lwt_current();
	__lwt_enqueue(&(rcving->senders_head), &(rcving->senders_tail), current);
	return rcving;
}

lwt_t lwt_create_chan(lwt_chan_fn_t fn, lwt_chan_t c){
	return NULL;
}
