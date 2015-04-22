#include <lwt.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#define ITER 10

void* send_func(lwt_chan_t c)
{
	int i;
	for(i = 1; i<=ITER; i++) lwt_snd(c, i);
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
		printf("boing %d\n", x);
		lwt_snd(s_snd, x);
	}
	lwt_chan_deref(start);
	lwt_chan_deref(rcv);
	lwt_chan_deref(s_snd);
}

void* rcv_func(lwt_chan_t c)
{
	int i;
	lwt_chan_t new_chan = lwt_chan(0);
	lwt_snd_chan(c, new_chan);
	for(i = 1; i<ITER; i++) assert(lwt_rcv(new_chan)==i);
	lwt_chan_deref(c);
	lwt_chan_deref(new_chan);
}

void dumb_func(lwt_chan_t dumb)
{
	printf("dumb function %d\n", dumb);
}
int main()
{
	int i, j;
	printf("Testing bullshit\n");
	lwt_chan_t receiver = lwt_chan(0);
	
	lwt_kthd_create(dumb_func, receiver);
	sleep(1);
	printf("done being dumb\n");
	/*lwt_t sender = lwt_create_chan(send_func, receiver, 0);
	
	for(i=1; i<=ITER; i++) assert(lwt_rcv(receiver)==i);
	lwt_join(sender);
	assert(receiver->snd_cnt == 0);

	printf("Testing send from main\n");
	lwt_t rcv_thd = lwt_create_chan(rcv_func, receiver, 0);
	lwt_chan_t snd_chan = lwt_rcv_chan(receiver);
	
	for(i=1; i<ITER; i++) lwt_snd(snd_chan, i);
	lwt_join(rcv_thd);
	assert(receiver->snd_cnt == 0);
	assert(snd_chan->rcv_deref);
	lwt_chan_deref(snd_chan);

	printf("Testing bounce\n");
	lwt_t bounce1 = lwt_create_chan(bounce, receiver, 0);
	lwt_t bounce2 = lwt_create_chan(bounce, receiver, 0);
	lwt_chan_t rcv1 = lwt_rcv_chan(receiver);
	lwt_chan_t rcv2 = lwt_rcv_chan(receiver);
	lwt_snd_chan(rcv1, rcv2);
	lwt_snd_chan(rcv2, rcv1);
	lwt_snd(rcv1, 1);
	lwt_join(bounce2);
	lwt_join(bounce1);
	assert(receiver->snd_cnt == 0);
	
	printf("Testing a bunch of senders\n");
	lwt_t senders[ITER];
	int counts[10];

	for(i = 0; i<ITER; i++){
		senders[i] = lwt_create_chan(send_func, receiver, 0);
		counts[i] = 0;
	}
	assert(receiver->snd_cnt == ITER);
	for(i = 0; i<ITER*ITER; i++){
		int rcv = lwt_rcv(receiver);
		counts[rcv-1]++;
	}
	for(i = 0; i<ITER; i++) assert(counts[i]==ITER);*/	
}
