/**
 * This program represents a basic pipeline pattern
 * 
 * The traces contain only the program instructions and data
 * accesses for the kernel_func for each thread
 * 
 * This program takes NUM_VALS random numbers and computes
 * x_i ^ NPROC for each one by having each core take 1 power
 * 
 * While inefficient, this models the pipeline pattern
 * in an understandable way
 * 
 * Note that this benchmark uses locks, each invocation
 * of lock() and unlock() triggers a getM until success
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>
#include <math.h>
#include <stdbool.h>

#define NPROC 8
#define NUM_VALS 4

// 64 byte aligned struct
struct workitem_struct {
    volatile int64_t partial_product;
    volatile int64_t base_value;
} __attribute__((aligned(64)));

typedef struct workitem_struct workitem;

// malloc isn't always threadsafe
pthread_mutex_t malloc_lock;

// a basic linked list backed queue {
typedef struct Node_struct {
    workitem *data;
    struct Node_struct *next;
} Node;

typedef struct Queue_struct {
    Node *head;
    Node *tail;
    size_t size;
} Queue;

Queue *queue_create() {
    pthread_mutex_lock(&malloc_lock);
    Queue * q = (Queue *) malloc(sizeof(Queue));
    pthread_mutex_unlock(&malloc_lock);
    q->head = NULL;
    q->tail = NULL;
    q->size = 0;
    return q;
}

void queue_push(Queue *queue, workitem *w) {
    pthread_mutex_lock(&malloc_lock);
    Node * n = (Node *) malloc(sizeof(Node));
    pthread_mutex_unlock(&malloc_lock);
    n->data = w;
    n->next = NULL;
    if (!queue->head) {
        // need to create head
        queue->head = n;
        queue->tail = n;
        queue->size++;
    } else {
        queue->tail->next = n;
        queue->tail = n;
        queue->size++;
    }
}

workitem *queue_pop(Queue *queue) {
    if (!queue->head) {
        return NULL;
    }

    workitem *w = queue->head->data;
    if (queue->head == queue->tail) {
        pthread_mutex_lock(&malloc_lock);
        free(queue->head);
        pthread_mutex_unlock(&malloc_lock);
        queue->head = NULL;
        queue->tail = NULL;
        queue->size--;
        return w;
    }

    Node * newhead = queue->head->next;
    pthread_mutex_lock(&malloc_lock);
    free(queue->head);
    pthread_mutex_unlock(&malloc_lock);
    queue->head = newhead;
    queue->size--;
    return w;
}
// } end queue implementation


int64_t truths[NUM_VALS];
volatile int64_t completed[NPROC + 1];

// every queue needs a lock since
// thread X reads stage X, writes stage X+1
pthread_mutex_t locks[NPROC + 1];
Queue *stages[NPROC + 1]; 

void *kernel_func(void *arg) {
    int64_t tid = (int64_t) arg;
    // loop until all elements processed
    while (completed[tid] < NUM_VALS) {
        workitem *w;
        pthread_mutex_lock(&locks[tid]);
        w = queue_pop(stages[tid]);
        pthread_mutex_unlock(&locks[tid]);
        if (w) {
            //there is a valid workitem
            w->partial_product *= w->base_value;
            pthread_mutex_lock(&locks[tid+1]);
            queue_push(stages[tid+1], w);
            pthread_mutex_unlock(&locks[tid+1]);
            completed[tid]++;
        }
    }
    return 0;
}

int main(void) {
    // setup data and right answer
    srand(0);
    pthread_mutex_init(&malloc_lock, NULL);
    for (int i = 0; i < NPROC + 1; i++) {
        pthread_mutex_init(&locks[i], NULL);
        stages[i] = queue_create();
        completed[i] = 0;
    }
    // using malloc'd workitems so that
    // pointer ends up moving through pipeline
    for (int i = 0; i < NUM_VALS; i++) {
        int64_t val = rand() % 16;
        workitem * w = (workitem *) malloc(sizeof(workitem));
        w->base_value = val;
        w->partial_product = 1;
        printf("Creating workitem: %ld\n", val);
        queue_push(stages[0], w);
        truths[i] = pow(val, NPROC);
    }

    
    pthread_t threads[NPROC];
    int retvals[NPROC];

    // spawn worker threads
    for (int64_t i = 0; i < NPROC; i++) {
        retvals[i] = pthread_create(&threads[i], NULL, kernel_func, (void *) i);
    }

    // join threads
    for (int i = 0; i < NPROC; i++) {
        pthread_join(threads[i], NULL);
    }

    // evaluate correctness
    bool correct = true;
    for (int i = 0; i < NUM_VALS; i++) {
        workitem *w = queue_pop(stages[NPROC]);
        if (w->partial_product != truths[i]) {
            printf("Mismatch! Got: %ld, Expected: %ld", w->partial_product, truths[i]);
            correct = false;
        }
    }

    if (correct) {
        printf("all values matched!\n");
    } else {
        printf("There was a mismatch!\n");
    }
}

