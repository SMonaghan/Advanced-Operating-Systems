/**
 * Redistribution of this file is permitted under the GNU General
 * Public License v2.
 *
 * Copyright 2012 by Gabriel Parmer.
 * Author: Gabriel Parmer, gparmer@gwu.edu, 2012
 */
/* 
 * This is a HTTP server.  It accepts connections on port 8080, and
 * serves a local static document.
 *
 * The clients you can use are 
 * - httperf (e.g., httperf --port=8080),
 * - wget (e.g. wget localhost:8080 /), 
 * - or even your browser.  
 *
 * To measure the efficiency and concurrency of your server, use
 * httperf and explore its options using the manual pages (man
 * httperf) to see the maximum number of connections per second you
 * can maintain over, for example, a 10 second period.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <sys/wait.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/mman.h>

#include "util.h" 		/* client_process */
#include "server.h"		/* server_accept and server_create */
#include "simple_http.h"

#include "common.h"
#include "ll.h"
#include "ring.h"
#include "../lwt_thread/lwt.h"

#define MAX_DATA_SZ 1024
#define MAX_CONCURRENCY 1
/* 
 * This is the function for handling a _single_ request.  Understand
 * what each of the steps in this function do, so that you can handle
 * _multiple_ requests.  Use this function as an _example_ of the
 * basic functionality.  As you increase the server in functionality,
 * you will want to probably keep all of the functions called in this
 * function, but define different code to use them.
 */

struct linked_list* ll;
extern const char success_head[];
void client_process_cache(int fd);
void group_wait(lwt_chan_t send_back);
void client_process_read(lwt_chan_t send);

int *counter = 0;
volatile int counter2 = 0;
pthread_mutex_t lock;

int cache[32];

extern char messages[32][129];
extern char messages_head[32][257];
char * mess = "wat u";

void message_test(void){
	strcpy(messages[0], "hallo thar");
	strcpy(messages[10], "Whats up doc");
	strcpy(messages[10], "Hey");

	printf("Mess1: %s Size: %d\nMess2: %s Size: %d\n", messages[0], strlen(messages[0]), messages[10], strlen(messages[10]));
}

void process(lwt_chan_t received){
	int fd, wait_iter;
	printf("thread created\n");
	lwt_chan_t receiver = lwt_chan(100);
	lwt_t waiter = lwt_create_chan(group_wait, receiver, LWT_NOJOIN);
	lwt_yield(LWT_NULL);
	lwt_snd_chan(received, receiver);
	while(1){
		fd = lwt_rcv(receiver);
		if(cache[fd % 32] == fd){
			printf("In cache\n");
			lwt_create(client_process_cache, fd, LWT_NOJOIN);
		}
		else{
			printf("Not in cache\n");
			lwt_chan_t rec = lwt_chan(100);
			cache[fd % 32] = fd;
			lwt_create(client_process_read, rec, LWT_NOJOIN);
		}
	}
}

void client_process_read(lwt_chan_t send){
	int fd;
	while(1){
		client_process(fd);
		lwt_snd(send, fd);
	}
}

void client_process_cache(int fd){
	int amnt_written = 0;
	struct http_req r;
	int resp_size = strlen(messages[32 % fd]), resp_hd_size = strlen(messages_head[32 % fd]);

	while (amnt_written != resp_hd_size) {
		int ret = write(fd, (char *) messages_head[fd % 32][amnt_written], 
				resp_hd_size - amnt_written);
		if (ret < 0) {
			printf("Could not write the response to the fd\n");
			goto done;
		}
		amnt_written += ret;
	}

	amnt_written = 0;
	while (amnt_written != resp_size) {
		int ret = write(fd, (char *) messages[fd % 32][amnt_written], 
				resp_size - amnt_written);
		if (ret < 0) {
			printf("Could not write the response to the fd\n");
			goto done;
		}
		amnt_written += ret;
	}
done:
	return;

}

void group_wait(lwt_chan_t send_back){
	int i;
	lwt_cgrp_t group = lwt_cgrp();
	lwt_chan_t wait_chans[5];
	for(i = 0; i<5; i++){
		wait_chans[i] = lwt_chan(10);
		lwt_cgrp_add(group, wait_chans[i]);
		lwt_kthd_create(client_process_read, wait_chans[i]);
	}
	while(1){
		lwt_chan_t c = lwt_cgrp_wait(group);
		lwt_snd(send_back, lwt_rcv(c));
	}
}

int server_thread_pool(int accept_fd){
	int fd;
	ll = createList();

	printf("start\n");
	lwt_chan_t receiver = lwt_chan(0);
	if(lwt_kthd_create(process, receiver)){
		printf("failed to create\n");
		return -1;
	}
	lwt_chan_t sender = lwt_rcv_chan(receiver);
	while(1){
		fd = server_accept(accept_fd);//waits until a request is accepted
		lwt_snd(sender, fd);
	}
	return 0;
}

pthread_cond_t full;
pthread_cond_t empty;
pthread_mutex_t block;
void ring_removes(struct ring* rb){
	//struct request *r;
	//int fd;
	printf("thread created\n");
	while(1)
	{
		pthread_mutex_lock(&block);
		while(rb_isempty(rb))
		{
			printf("child blocked\n");
			pthread_cond_wait(&empty, &block);
		}
		client_process(rb_dequeue(rb));
	}
}

typedef enum {
	SERVER_TYPE_ONE = 0,
	SERVER_TYPE_SINGLET,
	SERVER_TYPE_PROCESS,
	SERVER_TYPE_FORK_EXEC,
	SERVER_TYPE_SPAWN_THREAD,
	SERVER_TYPE_TASK_QUEUE,
	SERVER_TYPE_THREAD_POOL,
	SERVER_TYPE_THREAD_POOL_BLOCKING,
} server_type_t;

int main(int argc, char *argv[]){
	message_test();
	server_type_t server_type;
	short int port = 1234;
	int accept_fd;
	accept_fd = server_create(port);
	if (accept_fd < 0) return -1;

	printf("HERE\n");
	server_thread_pool(accept_fd);
	close(accept_fd);

	return 0;
}
