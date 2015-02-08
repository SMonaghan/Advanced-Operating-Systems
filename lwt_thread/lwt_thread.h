#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <sys/mman.h>
//#include <unistd.h>

#define STACK_SIZE (4 * 1024)//Size of the stack
#define LWT_NULL NULL//This is for the scheduling

unsigned int active_thread_count = 0;
unsigned int global_id = 0;

typedef enum {
	RUNNING,
	WAITING,
	READY,
} flag_t;

typedef struct thread {
	void * stack;
	unsigned int id;
	void * sp;
	struct thread *parent;
	struct thread *child;
	flag_t flag;
	void *value;
} *lwt_t;

typedef enum {
	LWT_INFO_NTHD_RUNNABLE,
	LWT_INFO_NTHD_BLOCKED,
	LWT_INFO_NTHD_ZOMBIES,
} lwt_info_t;

static lwt_t active_thread = NULL;

void __lwt_schedule(void);
void __lwt_dispatch(lwt_t next, lwt_t current);
void __lwt_trampoline(void);
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
	printf("start\n");
	active_thread_count = active_thread_count + 1;
	printf("counter incremented %d \n",active_thread_count);
	return active_thread_count;
}
lwt_t lwt_create(lwt_fn_t fn, void *data){
	printf("Start\n");	
	lwt_t lwt_thd = malloc(sizeof(struct thread));
	printf("Malloced!!!!!!!\n");	

	lwt_thd->stack = malloc(STACK_SIZE);
	printf("Stack Allocated!!!!!!!\n");	
	lwt_thd->id = global_id;//generate_id();
	global_id++;
	printf("ID Generated!!!!!!!\n");
	void* sp = lwt_thd->stack + STACK_SIZE;
	//lwt_thd->sp = (unsigned int) lwt_thd->stack + STACK_SIZE;

	*((int*) sp-1) = __lwt_trampoline;
	*((int*) sp-2) = sp;

	printf("two things on the stack %d: %d at %d and %d at %d\n", sp, *((int*) sp-1), (int*)sp-1,  *((int*) sp-2), (int*)sp-2);
	lwt_thd->sp = sp-32;
	printf("Stack Pointer set!!!!!!!\n");	
	//lwt_thd->parent = lwt_current();
	//printf("Parent Decided!!!!!!!\n");	
	lwt_thd->child = NULL;
	printf("Child Set!!!!!!!\n");	
	lwt_thd->value = NULL;
	printf("Value Set!!!!!!!\n");	

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

void __lwt_dispatch(lwt_t current, lwt_t next){

	//printf("PUSH START!!!!!!!!!!");
	asm volatile ("pushl %eax \n\t"
			"pushl %ebx \n\t"
			"pushl %ecx \n\t"
			"pushl %edx \n\t"
			"pushl %esi \n\t"
			"pushl %edi \n\t");

	asm volatile ("movl %%esp, %0 \n\t"
			"movl %1, %%esp \n\t"
			:"=r" (current->sp)
			:"r" (next->sp)
			:"memory");

	int i, j;
	for(i = 0; i<=8; i++)
		printf("stack %d: %d: %d\n", next->sp, (int*) next->sp+i, *((int*) next->sp+i));	


	asm volatile ("popl %edi \n\t"
			"popl %esi \n\t"
			"popl %edx \n\t"
			"popl %ecx \n\t"
			"popl %ebx \n\t"
			"popl %eax \n\t"
			"popl %ebp \n\t"
			"ret");

	asm volatile("movl %%esp, %0 \n\t"
			"mov %%edi, %1 \n\t"
			: "=r" (i), "=r" (j)
			: : );

	printf("stack pointer: %d, edi: %d\n", i, j);

	asm volatile("jmp %edi\n\t");

	//printf("\n");
	//printf("POP DONE!!!!!!!!!!");
}

void __lwt_trampoline(void){
	printf("boing\n");
	exit(0);
}

void * __lwt_stack_get(void){

}

void __lwt_stack_return(void *stk){

}


