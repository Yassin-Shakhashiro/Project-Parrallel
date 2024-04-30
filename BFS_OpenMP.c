#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <omp.h> 

#define ROW 6000
#define COL 6000
#define CACHE_LINE_PADDING 64 // Adjust padding based on your system's cache line size

typedef struct {
    int x;
    int y;
} Point;

typedef struct queueNode {
    Point pt;  // The coordinates of a cell
    int dist;  // cell's distance of from the source
    struct queueNode* next;
} QueueNode;

QueueNode* createNode(int x, int y, int dist) {
    QueueNode* newNode = (QueueNode*)malloc(sizeof(QueueNode));
    newNode->pt.x = x;
    newNode->pt.y = y;
    newNode->dist = dist;
    newNode->next = NULL;
    return newNode;
}

// Queue implementation
typedef struct {
    QueueNode *front, *rear;
} Queue;

void initQueue(Queue* q) {
    q->front = q->rear = NULL;
}

void enqueue(Queue* q, QueueNode* node) {
    if (q->rear == NULL) {
        q->front = q->rear = node;
        return;
    }
    q->rear->next = node;
    q->rear = node;
}

QueueNode* dequeue(Queue* q) {
    if (q->front == NULL)
        return NULL;

    QueueNode* temp = q->front;
    q->front = q->front->next;

    if (q->front == NULL)
        q->rear = NULL;

    return temp;
}

bool isEmpty(Queue* q) {
    return q->front == NULL;
}

// Check whether given cell (row, col) is a valid cell or not.
bool isValid(int row, int col) {
    return (row >= 0) && (row < ROW) && (col >= 0) && (col < COL);
}

int rowNum[] = {-1, 0, 0, 1};
int colNum[] = {0, -1, 1, 0};

int BFS(int mat[][COL], Point src, Point dest) {

    if (!mat[src.x][src.y] || !mat[dest.x][dest.y])
        return -1;

    // Padded visited array to reduce false sharing
    bool visited[ROW + CACHE_LINE_PADDING][COL]; 
    memset(visited, false, sizeof(visited));

    visited[src.x][src.y] = true;

    #pragma omp parallel
    {
        Queue q; 
        initQueue(&q);
        enqueue(&q, createNode(src.x, src.y, 0));

        while (!isEmpty(&q)) {
            QueueNode* curr = dequeue(&q);
            Point pt = curr->pt;

            if (pt.x == dest.x && pt.y == dest.y)
                return curr->dist;

            #pragma omp for schedule(dynamic, 50) // Dynamic scheduling with chunk size 50
            for (int i = 0; i < 4; i++) {
                int row = pt.x + rowNum[i];
                int col = pt.y + colNum[i];

                if (isValid(row, col) && mat[row][col] && !visited[row][col]) {
                    visited[row][col] = true;
                    enqueue(&q, createNode(row, col, curr->dist + 1));
                }
            } 
            free(curr); 
        } 
    }

    return -1;
}

int main() {
     int maze[ROW][COL];

    for (int i = 0; i < ROW; i++) {
        for (int j = 0; j < ROW; j++) {
            if (rand() % 5) // Roughly 80% open paths
                maze[i][j] = 1;
            else
                maze[i][j] = 0;
        }
    }

    // Set start and end positions as traversable
    maze[0][0] = 1;
    maze[ROW-1][COL-1] = 1;

    Point source = {0, 0};
    Point dest = {5999, 5999};
    
    clock_t begin = clock();

    int dist = BFS(maze, source, dest);

    clock_t end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;

    if (dist != -1)
        printf("Shortest Path is %d, Time taken: %f seconds\n", dist, time_spent);
    else
        printf("Shortest Path doesn't exist\n");

    return 0;
}
