#include <stdlib.h>
#include <stdio.h>
#include "lwt_thread.h"

void * demo_func(void *d){
	return 90;
}

int main(int argc, char argv[]){

	printf("start\n");
	lwt_t main = lwt_create(demo_func, NULL);
	main->flag = RUNNING;
	printf("%c",main->flag);
	printf("main created\n");
	lwt_t test1 = lwt_create(demo_func, NULL);
	lwt_t test2 = lwt_create(demo_func, NULL);

	printf("all created!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
	
	//test1->stack = malloc(4096);
	//test2->stack = malloc(4096);
	
	__lwt_dispatch(main, test1);

	printf("Dispatch one\n");
	int a = 5;
	int b = 10;
	int c = a*b;

	for(a=5; a<10; a++){
		c+=b;
	}

	printf("%d %d %d\n", a, b, c);
	__lwt_dispatch(test1, test2);

	printf("Dispatch two\n");
	int x = 5;
	int y = 10;
	int z = a*b;

	for(x=5; x<10; x++){
		z+=y;
	}
	printf("%d %d %d\n", x, y, z);

	__lwt_dispatch(test2, test1);
	printf("Dispatch three\n");
	printf("%d %d %d\n", a, b, c);

	printf("ID: %d\n",main->id);
	printf("ID: %d\n",test1->id);
	printf("ID: %d\n",test2->id);

	printf("Done\n");
}
