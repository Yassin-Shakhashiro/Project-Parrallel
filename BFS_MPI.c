#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <stdbool.h>

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
Queue* createQueue(int capacity) {
    Queue* queue = (Queue*)malloc(sizeof(Queue));
    queue->capacity = capacity;
    queue->front = queue->rear = -1;
    queue->array = (QueueNode*)malloc(queue->capacity * sizeof(QueueNode));
    return queue;
}

// Function to check if the queue is empty
bool isEmpty(Queue* queue) {
    return queue->front == -1;
}

// Function to enqueue a node into the queue
void enqueue(Queue* queue, QueueNode node) {
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
QueueNode dequeue(Queue* queue) {
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

// Function to perform BFS traversal
void bfs_mpi(int n, int start, int goal, int* maze, int comm_size, int comm_rank) {
    // Calculate chunk size
    int chunk_size = n / comm_size;
    int remainder = n % comm_size;

    // Adjust chunk size for master if there's a remainder
    if (comm_rank == MASTER) {
        chunk_size += remainder;
    }

    // Allocate memory for local portion of maze
    int* local_maze = (int*)malloc(chunk_size * n * sizeof(int));

    // Scatter maze data to all processes
    MPI_Scatter(maze, chunk_size * n, MPI_INT, local_maze, chunk_size * n, MPI_INT, MASTER, MPI_COMM_WORLD);

    // Perform BFS on local portion of maze
    int* visited = (int*)malloc(chunk_size * sizeof(int));
    for (int i = 0; i < chunk_size; i++) {
        visited[i] = 0;
    }

    // Create a queue for BFS traversal
    Queue* queue = createQueue(n);

    double start_time = MPI_Wtime();

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

    double end_time = MPI_Wtime();
    double elapsed_time = end_time - start_time;
    printf("Process %d: Time taken for BFS: %f seconds\n", comm_rank, elapsed_time);

    // Clean up memory
    free(local_maze);
    free(visited);
    free(queue->array);
    free(queue);
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int comm_rank, comm_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &comm_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);

    int n = 10; // Size of maze (assuming it's a square maze)
    int* maze = NULL;
    int goal = 5; // Goal node

    if (comm_rank == MASTER) {
        maze = (int*)malloc(n * n * sizeof(int));
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                maze[i * n + j] = rand() % 2; // Random 0s and 1s for simplicity
            }
        }
    }

    // Perform BFS using MPI
    bfs_mpi(n, 0, goal, maze, comm_size, comm_rank);

    // Clean up memory
    if (comm_rank == MASTER) {
        free(maze);
    }
    
    MPI_Finalize();
    return 0;
}