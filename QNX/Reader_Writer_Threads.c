#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>


#define LOG_FILE "system.log"


// Data packet structure
typedef struct {
    char *data;
    int size;
    unsigned long eventId;
    unsigned long eventCorrelationId;
} DataPacket;

// Queue node structure
typedef struct node {
    DataPacket packet;
    struct node *next;
} Node;

// Queue structure
typedef struct {
    Node *head, *tail;
    int count;
    sem_t full, empty; // Semaphores to manage full and empty states
    pthread_mutex_t lock; // Mutex for ensuring thread-safe access to the queue
} Queue;

Queue dataQueue; // Shared queue

// Basic logging function
void log_message(const char *message) {
    FILE *file = fopen(LOG_FILE, "a");
    if (file != NULL) {
        fprintf(file, "%s\n", message);
        fclose(file);
    }
}

// Function to initialize queue
void initializeQueue(Queue *q, int size) {
    q->head = q->tail = NULL;
    q->count = 0;
    sem_init(&q->full, 0, 0);
    sem_init(&q->empty, 0, size);
    pthread_mutex_init(&q->lock, NULL);
    log_message("Queue initialized.");
}

// Function to push data into the queue
void enqueue(Queue *q, DataPacket data) {
    Node *newNode = (Node*) malloc(sizeof(Node));
    if (newNode == NULL)
    {
    	log_message("Error: Memory allocation failed for new node.");
    	return; // Handle memory allocation failures
    }
    newNode->packet = data;
    newNode->next = NULL;

    sem_wait(&q->empty);
    pthread_mutex_lock(&q->lock);

    if (q->tail == NULL) {
        q->head = q->tail = newNode;
    } else {
        q->tail->next = newNode;
        q->tail = newNode;
    }
    q->count++;

    pthread_mutex_unlock(&q->lock);
    sem_post(&q->full);
    log_message("Data enqueued.");
}

// Function to pop data from the queue
DataPacket dequeue(Queue *q) {
    DataPacket data = {0};

    sem_wait(&q->full);
    pthread_mutex_lock(&q->lock);

    if (q->head == NULL) {
        pthread_mutex_unlock(&q->lock);
        log_message("Error: Tried to dequeue from an empty queue.");
        return data; // Return empty data if queue is unexpectedly empty
    }

    Node *temp = q->head;
    data = temp->packet;
    q->head = q->head->next;

    if (q->head == NULL) {
        q->tail = NULL;
    }
    q->count--;

    pthread_mutex_unlock(&q->lock);
    sem_post(&q->empty);

    free(temp); // Free the dequeued node
    log_message("Data dequeued.");
    return data;
}

// Function to simulate data retrieval from an external source
int get_external_data(char *buffer, int bufferSizeInBytes) {
    int val;
    char srcString[] = "0123456789abcdefghijklmnopqrstuvwxyxABCDEFGHIJKLMNOPQRSTUVWXYZ";

    val = (int)(rand() % bufferSizeInBytes);

    if (bufferSizeInBytes < val)
        return (-1);

    strncpy(buffer, srcString, val);

    return val;
}

// Function to process data
void process_data(char *buffer, int bufferSizeInBytes) {
    int i;

    if (buffer) {
        printf("thread %llu - ", pthread_self());
        for (i = 0; i < bufferSizeInBytes; i++) {
            printf("%c", buffer[i]);
        }
        printf("\n");
        memset(buffer, 0, bufferSizeInBytes);
    } else {
        printf("error in process data - %llu\n", pthread_self());
    }
}

// Writer thread
void *writer_thread(void *arg) {
    while (1) {
        DataPacket packet;
        packet.data = malloc(1024); // Allocate memory for the buffer
        packet.size = get_external_data(packet.data, 1024);
        packet.eventId = 0; // Assume event ID is not needed in this context
        packet.eventCorrelationId = 0; // Assume correlation ID is not needed in this context

        if (packet.size > 0) {
            enqueue(&dataQueue, packet);
        } else {
            free(packet.data); // Clean up if get_external_data failed
        }
    }
    return NULL;
}

// Reader thread
void *reader_thread(void *arg) {
    while (1) {
        DataPacket packet = dequeue(&dataQueue);
        if (packet.size > 0) {
            process_data(packet.data, packet.size);
            free(packet.data); // Free the data buffer after processing
        }
    }
    return NULL;
}

#define M 10
#define N 20

int main(int argc, char **argv) {
    pthread_t writers[N], readers[M];

    // Initialize the queue with a size limit (for example, 100 packets)
    initializeQueue(&dataQueue, 100);

    for (int i = 0; i < N; i++) {
        pthread_create(&writers[i], NULL, writer_thread, NULL);
    }

    for (int i = 0; i < M; i++) {
        pthread_create(&readers[i], NULL, reader_thread, NULL);
    }

    // Join threads (if needed)
    for (int i = 0; i < N; i++) {
        pthread_join(writers[i], NULL);
    }
    for (int i = 0; i < M; i++) {
        pthread_join(readers[i], NULL);
    }

    return 0;
}
