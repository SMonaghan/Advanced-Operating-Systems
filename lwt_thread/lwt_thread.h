#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <sys/mman.h>
//#include <unistd.h>

#define STACK_SIZE (4 * 1024)//Size of the stack
#define LWT_NULL NULL//This is for the scheduling

unsigned int * active_thread_count = 0;

typedef struct thread {
	void *stack;
	unsigned int id;
	unsigned int sp;
	struct thread *parent;
	struct thread *child;
	void *value;
} *lwt_t;

typedef enum {
	LWT_INFO_NTHD_RUNNABLE,
	LWT_INFO_NTHD_BLOCKED,
	LWT_INFO_NTHD_ZOMBIES,
} lwt_info_t;

static lwt_t active_thread;

void __lwt_schedule(void);
void __lwt_dispatch(lwt_t next, lwt_t current);
void * __lwt_stack_get(void);
void __lwt_stack_return(void *stk);

typedef void *(*lwt_fn_t)(void *);
lwt_t lwt_create(lwt_fn_t fn, void *data);
lwt_t lwt_current(void);
void * lwt_join(lwt_t thread);
void lwt_die(void * ret);
int lwt_yield(lwt_t thread);
int lwt_id(lwt_t thread);
int lwt_info(lwt_info_t t);

int generate_id(void){
	//active_thread_count = mmap(NULL, sizeof(unsigned int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	*active_thread_count = *active_thread_count + 1;
	return *active_thread_count;
}
lwt_t lwt_create(lwt_fn_t fn, void *data){
	lwt_t lwt_thd = malloc(sizeof(struct thread));
	
	lwt_thd->stack = malloc(STACK_SIZE);
	lwt_thd->id = generate_id();
	lwt_thd->sp = (unsigned int)&lwt_thd->stack;
	lwt_thd->parent = lwt_current();
	lwt_thd->child = NULL;
	lwt_thd->value = NULL;

	return lwt_thd;
}

void * lwt_join(lwt_t thread){

}

void lwt_die(void * ret){

}

int lwt_yield(lwt_t thread){

}

lwt_t lwt_current(void){
	return active_thread;
}

int lwt_id(lwt_t thread){
	return thread->id;
}

int lwt_info(lwt_info_t t){

}

void __lwt_schedule(void){
	lwt_t next_thread = active_thread;
	/*next_thread = ?*/
	/*put stuff here*/
	active_thread = next_thread;


}

void __lwt_dispatch(lwt_t next, lwt_t current){

}

void* __lwt_stack_get(void){

}

void __lwt_stack_return(void *stk){

}


