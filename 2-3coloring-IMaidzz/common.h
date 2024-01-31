/**
 * @file common.h
 * @author Maida Horozovic <e11927096@student.tuwien.ac.at>
 * @date 11.12.2023
 *
 * @brief Header file
 * 
 * @details holds all global variables and major data structures
 **/

//name of shared memory
#define SHM_NAME "shared_memory"

//name for the semaphore read
#define SEM_READ "sem_read"

//name for the semaphore write
#define SEM_WRITE "sem_write"

//name for the mutex semaphore
#define SEM_MUTEX "sem_mutex"

//maximum number if edges in a edge_list
#define MAX_NUM_OF_EDGES_IN_LIST 10

//maximum number of edges a solution should have
#define MAX_NUM 8

sem_t *sem_write;
sem_t *sem_read;
sem_t *sem_mutex;

enum color{
    BLUE = 1,
    GREEN = 2,
    RED = 3
};

typedef struct 
{
    int number;
    int col;
}edge;


typedef struct
{
    edge from;
    edge to;
}vertex;

typedef struct
{
    vertex edges[MAX_NUM_OF_EDGES_IN_LIST];
    int num_of_vertices;
}edge_list;


/**
 * Structure of the circular buffer
 * @brief Internal structure of the circular buffer
 * @details Only the shm struct inside the circbuf struct is shared with other 
 * processes
 */
struct cirula_buffer
{
    edge_list lists [MAX_NUM];
    int read_index;
    int write_index;
    int terminate;
    int limit;
    int total_num_of_edges;
};
