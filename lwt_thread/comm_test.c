#include <lwt.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#define ITER 1000

void* send_func(lwt_chan_t c)
{
	int i;
	for(i = 1; i<=ITER; i++){
		//printf("sending %d from %d\n", i, ktcb);
		lwt_snd(c, i);
	}
	lwt_chan_deref(c);
	return i;
}

void bounce(lwt_chan_t start)
{
	int i, x;
	lwt_chan_t rcv = lwt_chan(0);
	lwt_snd_chan(start, rcv);
	lwt_chan_t s_snd = lwt_rcv_chan(rcv);
	for(i = 1; i<ITER; i++){
		x = lwt_rcv(rcv);
		x++;
		//printf("boing %d\n", x);
		lwt_snd(s_snd, x);
	}
	lwt_snd(start, i);
	lwt_chan_deref(start);
	lwt_chan_deref(rcv);
	lwt_chan_deref(s_snd);
}

void* rcv_func(lwt_chan_t c)
{
	int i;
	lwt_chan_t new_chan = lwt_chan(0);
	lwt_snd_chan(c, new_chan);
	for(i = 1; i<ITER; i++){
		volatile int r = lwt_rcv(new_chan);
		if(i!=r) printf("fail, i = %d, r = %d\n", i, r);
		else printf("success i = %d, r = %d\n", i, r);
	}
	lwt_chan_deref(c);
	lwt_chan_deref(new_chan);
}

void dumb_func(lwt_chan_t dumb)
{
	printf("dumb function %d\n", dumb);
}

void *
fn_grpwait(lwt_chan_t c)
{
	int i;

	for (i = 0 ; i < ITER ; i++) {
		if ((i % 7) == 0) {
			int j;

			for (j = 0 ; j < (i % 8) ; j++) lwt_yield(LWT_NULL);
		}
		lwt_snd(c, ktcb);
	}
}

#define GRPSZ 3
void
test_grpwait(int chsz, int grpsz)
{
	lwt_chan_t cs[grpsz];
	int i;
	lwt_cgrp_t g;

	printf("[TEST] group wait (channel buffer size %d, grpsz %d)\n", 
	       chsz, grpsz);
	g = lwt_cgrp();
	assert(g);
	
	for (i = 0 ; i < grpsz ; i++) {
		cs[i] = lwt_chan(chsz);
		assert(cs[i]);
		lwt_kthd_create(fn_grpwait, cs[i]);
		//lwt_chan_mark_set(cs[i], (void*)lwt_id(ts[i]));
		lwt_cgrp_add(g, cs[i]);
	}
	assert(lwt_cgrp_free(g) == -1);
	for (i = 0 ; i < (ITER * grpsz) ; i++) {
		lwt_chan_t c;
		int r;

		c = lwt_cgrp_wait(g);
		assert(c);
		r = (int)lwt_rcv(c);
		//printf("receiving %d\n", r);
		//assert(r == (int)lwt_chan_mark_get(c));
	}
	for (i = 0 ; i < grpsz ; i++) {
		lwt_cgrp_rem(g, cs[i]);
		//lwt_join(ts[i]);
		lwt_chan_deref(cs[i]);
	}
	assert(!lwt_cgrp_free(g));
	
	return;
}

int main()
{
	int i, j, ret;
	lwt_chan_t receiver = lwt_chan(0);
	
	lwt_kthd_create(dumb_func, receiver);
	//sleep(1);
	printf("done being dumb\n");

	/*for(i = 1; i<135; i++){	
		ret = __lwt_rb_add_wf(ktcb->wf_rb, i);
		printf("i = %d, Status: %d, added %d\n", i, ret, i);
	}
	for(i = 1; i<135; i++){
		ret = __lwt_rb_rem_wf(ktcb->wf_rb);
		printf("Got %d\n", ret);
	}*/	
	
	test_grpwait(0,3);
	test_grpwait(3,3);
	printf("passed group wait\n");	
	//lwt_join(sender);
	//assert(receiver->snd_cnt == 0);

	sleep(1); 
	
	printf("Testing send from main\n");
	lwt_kthd_create(rcv_func, receiver);
	lwt_chan_t snd_chan = lwt_rcv_chan(receiver);
	
	for(i=1; i<ITER; i++) lwt_snd(snd_chan, i);
	//lwt_join(rcv_thd);
	//assert(receiver->snd_cnt == 0);
	//assert(snd_chan->rcv_deref);
	//lwt_chan_deref(snd_chan);
	
	sleep(1);
	printf("Testing bounce\n");
	lwt_kthd_create(bounce, receiver);
	lwt_kthd_create(bounce, receiver);
	lwt_chan_t rcv1 = lwt_rcv_chan(receiver);
	lwt_chan_t rcv2 = lwt_rcv_chan(receiver);
	lwt_snd_chan(rcv1, rcv2);
	lwt_snd_chan(rcv2, rcv1);
	lwt_snd(rcv1, 1);
	assert(lwt_rcv(receiver)==ITER);
	assert(lwt_rcv(receiver)==ITER);
	printf("passed bounce\n");
	//lwt_join(bounce2);
	//lwt_join(bounce1);
	//assert(receiver->snd_cnt == 0);

	sleep(1);
	printf("Testing a bunch of senders\n");
	volatile int counts[ITER];

	for(i = 0; i<10; i++) lwt_kthd_create(send_func, receiver);
	for(i = 0; i<ITER; i++) counts[i] = 0;
	for(i = 0; i<10*ITER; i++){
		int rcv = lwt_rcv(receiver);
		printf("receiving %d, i = %d\n", rcv, i);
		counts[rcv-1]++;
	}
	for(i = 0; i<ITER; i++){
		if(counts[i] != 10) printf("fail at %d, count %d\n", i, counts[i]);	
	}
	printf("pass!\n");	
}
