/**
 * This program creates an array of length PARTIAL * NPROC 
 * and attempts to take the sum of it
 *
 * The traces contain only the program instructions and data
 * accesses for the kernel_func for each thread
 *
 * The array_stripe benchmark distributes the work for each
 * thread interleaved in the indices
 * 
 * Note that the 2 array benchmarks perform the same work
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#define NPROC 4

#define PARTIAL 64
#define SIZE (NPROC * PARTIAL)

int vals[SIZE];

volatile int partial_sums[NPROC];
int global_sum;

void *kernel_func(void * arg) {
    int64_t tid = (int64_t) arg;
    int local_sum = 0;
    for (int i = tid; i < SIZE; i+=NPROC) {
        local_sum += vals[i];
    }
    partial_sums[tid] = local_sum;
}

int main(void) {

    // setup data and compute correct answer
    srand(0);
    int check = 0;
    for (int i = 0; i < SIZE; i++) {
        vals[i] = rand() % 16;
        check += vals[i];
    }
    printf("sum should be: %d\n", check);
    
    pthread_t threads[NPROC];
    int retvals[NPROC];

    // Spawn the worker threads
    for (int64_t i = 0; i < NPROC; i++) {
        retvals[i] = pthread_create(&threads[i], NULL, kernel_func, (void *) i);
    }

    // Wait for threads to complete
    for (int i = 0; i < NPROC; i++) {
        pthread_join(threads[i], NULL);
    }

    // reduction
    for (int i = 0; i < NPROC; i++) {
        global_sum += partial_sums[i];
    }
    printf("parallel computed sum: %d\n", global_sum);

    if (check != global_sum) {
        printf("Mismatch!\n");
    } else {
        printf("Matched!\n");
    }
}
