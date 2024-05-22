/**
 * This program is a trivial map-shuffle-reduce kernel
 * 
 * The traces contain only the program instructions and data
 * accesses for the kernel_func for each thread
 * 
 * The MAP+SHUFFLE phase has each core generate values 
 *      between 0-64 and add the generated value x_i 
 *      to the queue for core (x_i % NPROC)
 * 
 * Then, the REDUCE phase has each core count the values
 *      received and generate some summary statistics
 * 
 * A final reduction step takes the aggregate from each
 * thread
 * 
 * Note that this benchmark uses locks, each invocation
 * of lock() and unlock() triggers a getM until success
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>

#define NPROC (16)
#define VALS_PER_PROC (32 / NPROC)
#define MAX_VAL_MOD (64)

// 64 byte aligned struct
struct result_struct {
    volatile int64_t count;
    volatile int64_t sum;
} __attribute__((aligned(64))); // struct padded to 64 bytes

typedef struct result_struct result;

// malloc isn't always threadsafe
pthread_mutex_t malloc_lock;

// a basic linked list backed queue {
typedef struct Node_struct {
    int data;
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

void queue_push(Queue *queue, int data) {
    pthread_mutex_lock(&malloc_lock);
    Node * n = (Node *) malloc(sizeof(Node));
    pthread_mutex_unlock(&malloc_lock);
    n->data = data;
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

int queue_pop(Queue *queue) {
    if (!queue->head) {
        return -1;
    }

    int data = queue->head->data;
    if (queue->head == queue->tail) {
        pthread_mutex_lock(&malloc_lock);
        free(queue->head);
        pthread_mutex_unlock(&malloc_lock);
        queue->head = NULL;
        queue->tail = NULL;
        queue->size--;
        return data;
    }

    Node * newhead = queue->head->next;
    pthread_mutex_lock(&malloc_lock);
    free(queue->head);
    pthread_mutex_unlock(&malloc_lock);
    queue->head = newhead;
    queue->size--;
    return data;
}
// } end queue implementation

// every queue needs a lock since multiple cores
// need to write to each queue at different times
pthread_mutex_t locks[NPROC];
Queue *queues[NPROC];
result phase2_out[NPROC];
result reduction;

// using an int to implement a barrier is inefficient
// but more efficient things are more complicated to
// explain with coherence
pthread_mutex_t phase1_lock;
volatile int phase1_completed = 0;
pthread_mutex_t phase2_lock;
volatile int phase2_completed = 0;


void *kernel_func(void *arg) {
    int64_t tid = (int64_t) arg;
    unsigned int rand_state = (unsigned int) tid;
    // map+shuffle
    for (int i = 0; i < VALS_PER_PROC; i++) {
        int val = rand_r(&rand_state) % (MAX_VAL_MOD);
        int dest = val % NPROC;
        pthread_mutex_lock(&locks[dest]);
        queue_push(queues[dest], val);
        pthread_mutex_unlock(&locks[dest]);
    }
    pthread_mutex_lock(&phase1_lock);
    phase1_completed = phase1_completed + 1;
    pthread_mutex_unlock(&phase1_lock);
    // wait till all threads are done (barrier)
    while (phase1_completed != NPROC);

    bool done = false;
    while (!done) {
        // only using your own queue so no locks
        int popped = queue_pop(queues[tid]);
        if (popped != -1) {
            phase2_out[tid].count++;
            phase2_out[tid].sum += popped;
        } else {
            done = true;
        }
    }
    pthread_mutex_lock(&phase2_lock);
    phase2_completed = phase2_completed + 1;
    pthread_mutex_unlock(&phase2_lock);

    // another barrier
    while (phase2_completed != NPROC);

    // final reduction
    if (tid == 0) {
        for (int i = 0; i < NPROC; i++) {
            reduction.count += phase2_out[i].count;
            reduction.sum += phase2_out[i].sum;
        }
    }
    return 0;
}

int main(void) {
    pthread_mutex_init(&malloc_lock, NULL);
    pthread_mutex_init(&phase1_lock, NULL);
    pthread_mutex_init(&phase2_lock, NULL);
    for (int i = 0; i < NPROC; i++) {
        pthread_mutex_init(&locks[i], NULL);
        queues[i] = queue_create();
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

    printf("total values: %ld, sum: %ld\n", reduction.count, reduction.sum);
}