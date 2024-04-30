#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h> 

#define MASTER 0

// Structure to represent a node in the queue
typedef struct {
    int value;
    int process_rank;
} QueueNode;

// Structure to represent a queue
typedef struct {
    QueueNode* array;
    int front, rear;
    int capacity;
} Queue;

// Function to create a new queue
_device_ Queue* createQueue(int capacity) {
    Queue* queue = (Queue*)malloc(sizeof(Queue));
    queue->capacity = capacity;
    queue->front = queue->rear = -1;
    queue->array = (QueueNode*)malloc(queue->capacity * sizeof(QueueNode));
    return queue;
}

// Function to check if the queue is empty
_device_ bool isEmpty(Queue* queue) {
    return queue->front == -1;
}

// Function to enqueue a node into the queue
_device_ void enqueue(Queue* queue, QueueNode node) {
    if (queue->rear == queue->capacity - 1) {
        printf("Queue overflow\n");
        return;
    }
    queue->array[++queue->rear] = node;
    if (queue->front == -1) {
        queue->front = 0;
    }
}

// Function to dequeue a node from the queue
_device_ QueueNode dequeue(Queue* queue) {
    if (isEmpty(queue)) {
        printf("Queue underflow\n");
        QueueNode emptyNode = {-1, -1};
        return emptyNode;
    }
    QueueNode node = queue->array[queue->front];
    if (queue->front == queue->rear) {
        queue->front = queue->rear = -1;
    } else {
        queue->front++;
    }
    return node;
}

// Kernel function to perform BFS traversal
_global_ void bfs_kernel(int n, int start, int goal, int* maze, int chunk_size) {
    int comm_rank = blockIdx.x;
    
    // Calculate offset for the local portion of maze
    int offset = comm_rank * chunk_size * n;
    int start_index = offset + start;
    
    // Allocate memory for local portion of maze
    int* local_maze = (int*)malloc(chunk_size * n * sizeof(int));
    for (int i = 0; i < chunk_size; i++) {
        for (int j = 0; j < n; j++) {
            local_maze[i * n + j] = maze[start_index + i * n + j];
        }
    }
    
    // Perform BFS on local portion of maze
    int* visited = (int*)malloc(chunk_size * sizeof(int));
    for (int i = 0; i < chunk_size; i++) {
        visited[i] = 0;
    }
    
    // Create a queue for BFS traversal
    Queue* queue = createQueue(n);
    
    // Enqueue the starting node
    QueueNode startNode = {start, comm_rank};
    enqueue(queue, startNode);
    visited[start] = 1;
    
    // Perform BFS traversal
    while (!isEmpty(queue)) {
        QueueNode currentNode = dequeue(queue);
        int current = currentNode.value;
        
        // Check if current node is the goal
        if (current == goal) {
            printf("Process %d: Found goal at node %d\n", comm_rank, current);
            break;
        }
        
        for (int i = 0; i < n; i++) {
            if (local_maze[current * n + i] == 1 && !visited[i]) {
                QueueNode nextNode = {i, comm_rank};
                enqueue(queue, nextNode);
                visited[i] = 1;
            }
        }
    }
    
    // Clean up memory
    free(local_maze);
    free(visited);
    free(queue->array);
    free(queue);
}

int main() {
    int n = 6000; // Size of maze (assuming it's a square maze)
    int* maze = (int*)malloc(n * n * sizeof(int));
    int goal = 5999; // Goal node
    
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            maze[i * n + j] = rand() % 2; // Random 0s and 1s for simplicity
        }
    }
    
    int comm_size = n; // Assuming each process handles one row of the maze
    int chunk_size = 1; // Each process handles one row of the maze
    
    // Timing variables
    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);
    
    // Perform BFS using CUDA
    bfs_kernel<<<comm_size, 1>>>(n, 0, goal, maze, chunk_size);
    cudaDeviceSynchronize();
    
    // Timing
    gettimeofday(&end_time, NULL);
    double execution_time = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_usec - start_time.tv_usec) / 1e6; // Convert to seconds
    
    printf("Total execution time: %f seconds\n", execution_time);
    
    // Clean up memory
    free(maze);
    
    return 0;
}