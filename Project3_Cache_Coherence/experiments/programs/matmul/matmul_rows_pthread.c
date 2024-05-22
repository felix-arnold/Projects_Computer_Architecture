/**
 * This program creates a set of 3 matrices, each with 
 * dimension(s) dependent on NPROC and takes the product
 * 
 * The traces contain only the program instructions and data
 * accesses for the kernel_func for each thread
 * 
 * The matmul benchmark distributes the work for each
 * thread between the rows of the a and c matrices
 *
 * Note that both matmul benchmarks perform the same work
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#define NPROC 4

#define ROWS_PER_PROC 4
#define I (NPROC * ROWS_PER_PROC)

#define J 8

#define K (NPROC * ROWS_PER_PROC)

int a[I][J];
int b[J][K];
int c[I][K];
int truth[I][K];

void *kernel_func(void * arg) {
    int64_t tid = (int64_t) arg;
    for (int i = tid; i < I; i+=NPROC) { // interleave threads per core
        for (int j = 0; j < J; j++) {
            for (int k = 0; k < K; k++) {
                c[i][k] += a[i][j] * b[j][k];
            }
        }
    }
}

int main(void) {

    // setup data and compute correct product
    srand(0);
    for (int i = 0; i < I; i++) {
        for (int j = 0; j < J; j++) {
            a[i][j] = rand() % 16;
        }
    }
    for (int j = 0; j < J; j++) {
        for (int k = 0; k < K; k++) {
            b[j][k] = rand() % 16;
        }
    }

    for (int i = 0; i < I; i++) {
        for (int j = 0; j < J; j++) {
            for (int k = 0; k < K; k++) {
                truth[i][k] += a[i][j] * b[j][k];
            }
        }
    }

    printf("Product should be\n");
    for (int i = 0; i < I; i++) {
        for (int k = 0; k < K; k++) {
            printf("%d, ", truth[i][k]);
        }
        printf("\n");
    }

    printf("\n\n");
    
    pthread_t threads[NPROC];
    int retvals[NPROC];

    // spawn worker threads
    for (int64_t i = 0; i < NPROC; i++) {
        retvals[i] = pthread_create(&threads[i], NULL, kernel_func, (void *) i);
    }

    for (int i = 0; i < NPROC; i++) {
        pthread_join(threads[i], NULL);
    }

    // check answer
    int error = 0;
    printf("parallel computed product\n");
    for (int i = 0; i < I; i++) {
        for (int k = 0; k < K; k++) {
            printf("%d, ", c[i][k]);
            if (truth[i][k] != c[i][k]) {
                error = 1;
            }
        }
        printf("\n");
    }

    if (error) {
        printf("computation mismatch occured\n");
    } else {
        printf("identical results!\n");
    }

}