#include "slab.h"
#include <stdlib.h>
#include <stdio.h>

struct testObj1{
    int w, x, y, z;
};
struct testObj2{
    int opus[50];
};
struct testObj3{
    int magnumOpus[100][100];
};

int main(){
    
    int i;
    
    kmem_init();
    
    struct kmem_cache* test1 = kmem_cache_create("test1", sizeof(struct testObj1));
    struct kmem_cache* test2 = kmem_cache_create("test2", sizeof(struct testObj2));
    struct kmem_cache* test3 = kmem_cache_create("test3", sizeof(struct testObj3));
    
    printf("Caches created %d %d %d\n", sizeof(struct testObj1), sizeof(struct testObj2), sizeof(struct testObj3));
    
    //print_cache_info(test1);
    //print_cache_info(test2);
    //print_cache_info(test3);
    
    struct testObj1* tester1[750]; 
    struct testObj2* tester2[35];
    struct testObj3* tester3[2];
    
    tester1[0] = kmem_cache_alloc(test1);
    tester2[0] = kmem_cache_alloc(test2);
    tester3[0] = kmem_cache_alloc(test3);
    
    printf("\nObjects allocated %d %d %d\n", tester1[0], tester2[0], tester3[0]);
    
    tester1[0]->w = 5;
    tester2[0]->opus[0] = 10;
    tester3[0]->magnumOpus[0][0] = 15;
    /*print_cache_info(test1);
    print_cache_info(test2);
    print_cache_info(test3);*/
    
    printf("\nData manipulated\n");
    printf("test1: %d\ttest2: %d\ttest3: %d\n", tester1[0]->w, tester2[0]->opus[0], tester3[0]->magnumOpus[0][0]);
    
    for(i = 1; i<500; i++){
        tester1[i] = kmem_cache_alloc(test1);
        //printf("test1 %d %d\n", i, tester1[i]);
    }
    for(i = 1; i<35; i++){
        tester2[i] = kmem_cache_alloc(test2);
        //printf("test2 %d %d\n", i, tester2[i]);
    }
    for(i = 1; i<2; i++){
        tester3[i] = kmem_cache_alloc(test3);
        //printf("test3 %d %d\n", i, tester3[i]);
    }
    
    printf("%d %d %d\n", tester1[399], tester2[5], tester3[1]);
    
    tester1[399]->x = 1;
    tester2[5]->opus[0] = 1;
    tester3[1]->magnumOpus[0][0] = 1;
    
    printf("\nMore allocation and manipulation\n");
    printf("test1: %d\ttest2: %d\ttest3: %d\n", tester1[399]->x, tester2[5]->opus[0], tester3[1]->magnumOpus[0][0]);
    
    printf("\nFreeing objects\n");
    
    kmem_cache_free(test1, tester1[1]);
    //kmem_cache_free(test2, tester2[1]);
    kmem_cache_free(test3, tester3[1]);
    
    for(i = 0; i<255; i++){
        kmem_cache_free(test1, tester1[i]);
    }
    //printf("Trouble: %d\n", tester1[255]);
    for(i = 2; i<35; i++){
        //printf("Freeing %d %d\n", i, tester2[i]);
        kmem_cache_free(test2, tester2[i]);
    }
    //kmem_cache_free(test3, tester3[1]);
    
    printf("\nObjects freed\n");
    
    print_cache_info(test1);
    print_cache_info(test2);
    print_cache_info(test3);
    
    printf("\nOverfilling test1 again\n");
    for(i = 500; i<750; i++){
        tester1[i] = kmem_cache_alloc(test1);
    }
    
    print_cache_info(test1);
    /*
    //ll_print(test2->freelist);
    
    printf("\nTesting double free on %d\n", tester2[1]);
    kmem_cache_free(test2, tester2[1]);
    
    print_cache_info(test1);
    print_cache_info(test2);
    print_cache_info(test3);*/
    return 0;
}