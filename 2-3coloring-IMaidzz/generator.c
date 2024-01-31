/**
 * @file generator.c
 * @author [Dein ? Vorname] [Dein ? Nachname] <matrikelnummer@student.tuwien.ac.at>
 * @date 11.12.2023
 *
 * @brief Generator program
 * 
 * @details Continuously generates 3-coloring solutions to a graph
 * and writes those to a supervisor program
 * till it is terminated by the supervisor program
 **/

#include <ctype.h>
#include <getopt.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>

#include "common.h"

/**
 * @brief variable name of program
 * used to inform the used of which program is currently executing which process
 */
char *prog_name;

/**
 * @brief function exits program when error occurs
 * @details exits program with status of EXIT_FAILURE and prints to user what went wrong
 * @param msg is the msg that the user will rexieve of what part of the program exactly expirienced an error
 */
static void error_eit(char *msg) {
    fprintf(stderr, "%s : %s\n", prog_name, msg);
    exit(EXIT_FAILURE);
}

/**
 * @brief function returns correct usage of the program
 * @details functions prints on stdout the USAGE and examples of the program
*/
static void usage(void){
    fprintf(stdout, "USAGE: ./generator EDGE1...n\n");
    fprintf(stdout, "The program takes in a list of edges specified as positional arguments\n");
    fprintf(stdout, "EXAMPLE generator 0-1 0-2 0-3 1-2 1-3 2-3\n");
}

/**
 * @brief indicator whather or not the program should terminate
*/
static volatile sig_atomic_t quit = 0;

/**
 * @brief the function changes the variable quit to true
 * @details this function is usualy called after it recieves a signal
 * @param signal it is the signal the program recieves externaly
*/
static void handle_signal(int signal)
{
	quit = 1;
}

/**
 * @brief opens all resources for progam to work
 * @details only works if supervisor was previously called, 
 * opens shared memory as a circular buffer,semaphores
 * @param buffer is the circular buffer that needs to be initialised in this function, the shared memory
 * @param shmfd is the link to the memory location of the shared memory
*/
static void opening_resources(struct cirula_buffer **buffer, int *shmfd) {
    *shmfd = shm_open(SHM_NAME, O_RDWR | O_CREAT, 0600);

    if (*shmfd == -1) {
        error_eit("Program failed to open shared memory");
    }

    *buffer = mmap(NULL, sizeof(**buffer), PROT_READ | PROT_WRITE, MAP_SHARED, *shmfd, 0);

    if (*buffer == MAP_FAILED) {
        error_eit("Program failed while mapping memory");
    }

    if (close(*shmfd) < -1) {
        error_eit("Program failed to close shared memory link");
    }

    sem_read = sem_open(SEM_READ, 0);
    // Initialize to 1 to allow the first write

    if (sem_read == SEM_FAILED) {
        error_eit("Failed opening the semaphore read");
    }

    sem_write = sem_open(SEM_WRITE, 0);
    if (sem_write == SEM_FAILED) {
        error_eit("Failed opening the semaphore write");
    }

    sem_mutex = sem_open(SEM_MUTEX, 0);
    if (sem_mutex == SEM_FAILED) {
        error_eit("Failed opening the semaphore mutex");
    }
}

/**
 * @brief cleans up all resources opened with 'opening resources' function
 * @details closes shared memory ,semaphores and inrements the sem_mutes as to 
 * make all remaining generators terminate in case one is still hanging in the queue
 * @param buffer is the shared memory buffer
*/
static void clean_up(struct cirula_buffer *buffer) {

    //post semaphores to avoid deadlock

    sem_post(sem_mutex);

    //to ensure that all processes end
    buffer->terminate = 1;
    quit = 1;

    if (munmap(buffer, sizeof(*buffer)) == -1) {
        error_eit("Program failed unmapping memory");
    }

    if (shm_unlink(SHM_NAME) < -1) {
        error_eit("Program failed unlinking of shared memory");
    }

    if (sem_close(sem_read) < -1) {
        error_eit("Failed closing the read semaphore");
    }

    if (sem_unlink(SEM_READ) < -1) {
        error_eit("Failed unlinking the read semaphore");
    }

    if (sem_close(sem_write) < -1) {
        error_eit("Failed unliking the read semaphore");
    }

    if (sem_unlink(SEM_WRITE) < -1) {
        error_eit("Failed unlinking the read semaphore");
    }

    if (sem_close(sem_mutex) < -1) {
        error_eit("Failed unliking the read semaphore");
    }

    if (sem_unlink(SEM_MUTEX) < -1) {
        error_eit("Failed unlinking the read semaphore");
    }

    // Free the memory allocated with malloc
}



/**
 * @brief takes and processes the inputed edges
 * @details takes a string representings the edges of the program 
 * parses them and makes sure that the input is correct
 * @param list takes an empty list of vertices that is to be filled by the funtction
 * @param argc is the number if  positonal arguments and therefore also vertices
 * @param argv are the positional arguments that are being handed over from main
 * @param vertex1 is the vertex that is used to temporary process the edges
*/
static void get_input(vertex **list, int argc, char *argv[], vertex *vertex1) {

    //starting from 1 beause the first argument is always the program name
    for (size_t i = 1; i < argc ; i++) {
        char *second = NULL;
        char *pos_of_minus = strchr(argv[i], '-');
        //check if correct format
        if (pos_of_minus == NULL)
        {
            usage();
            error_eit("Wrong formating of input arguments");
        }

        if (pos_of_minus == argv[i])
        {
            usage();
            error_eit("Missing first edge in an vertex");
        }
        
        vertex1->from.number = abs(strtol(argv[i], &second, 10));

        if (second == NULL)
        {
            usage();
            error_eit("No second paramether for edge given");
        }
        
        vertex1->to.number = abs(strtol(second, NULL, 10));

        (*list)[i] = *vertex1;
    }

}

/**
 * @brief checks if an edge is inside a list of edges
 * @details if the edge is inside the list it returns 1 and if not -1
 * @param edges the list of possible edges
 * @param num_of_balls is the number of edges
 * @param edge is the edge that we are trying to find in the list
 * @return either values 1 or -1 depending whather it is inside the list or not
*/
static int ball_inside(edge *edges, int num_of_balls, edge edge) {
    for (size_t i = 0; i < num_of_balls; i++) {
        
        if (edges[i].number == edge.number) {
            return 1;
        }
    }
    return -1;
}


/**
 * @brief gets all the edges from a  list of vertices
 * @details takes in an empty list of edges and fills it 
 * with the edges it find in the list of vertices
 * @param edges empty list of edges
 * @param num_of_balls is the number of edges that is found in the function
 * it is incremented every time a new edge is found and starts at 0
 * @param list is the list of vertices
 * @param argc is the number of vertices
*/
static void get_all_balls(edge **edges, int *num_of_balls, vertex *list, int argc) {
    for (size_t i = 0; i < argc - 1; i++) {
        if (ball_inside(*edges, *num_of_balls, list[i].from) == -1) {
            (*edges)[*num_of_balls] = list[i].from;
            (*num_of_balls)++;
        }

        if (ball_inside(*edges, *num_of_balls, list[i].to) == -1) {
            (*edges)[*num_of_balls] = list[i].to;
            (*num_of_balls)++;
        }
    }
}

/**
 * @brief finds a vertex in a list of vertices
 * @details find a vertex in a list of vertices and returns its value, it finds it 
 * by searching for a  vertex that has two edges that are specified
 * @param e1 is of the edges that need to be in the vertex
 * @param e2 is the other edge that needs to be in the vertex
 * @param list is the list of vertices
 * @param argc is the number of vertices
 * @return is returning the vertex if it find one and if not it returns a vertex with both edges set to -1 
*/
static vertex get_vertex(edge e1, edge e2, vertex *list, int argc) {
    vertex v1;
    v1.from.number = -1;
    v1.to.number = -1;

    // Check if the current element is initialized

    for (size_t i = 0; i < argc; i++) {

        if (list[i].from.number == -1 || list[i].to.number == -1) {
            continue;  // Skip uninitialized elements
        }

        if (list[i].from.number == e1.number && list[i].to.number == e2.number) {
            v1.from = e1;
            v1.to = e2;
            break;  // break out of the loop once the vertex is found
        }
    }
    return v1;
}

/**
 * @brief writes to the buffer
 * @details writes to the circular buffer after waiting for the permission to write by the supervisor
 * afterwards it also increments the writing index in the buffer
 * @param list is the edge list that needs to be written to the buffer
 * @param buffer is the shared memory buffer
*/
static void write_buffer(edge_list *list, struct cirula_buffer *buffer) {

    int write_index = buffer->write_index;

    // Copy the data from list to the shared memory buffer
    (*buffer).lists[write_index].num_of_vertices = list->num_of_vertices;
    memcpy(buffer->lists[write_index].edges, list->edges, sizeof(edge) * list->num_of_vertices);

    buffer->write_index = (write_index + 1) % MAX_NUM;

    sem_post(sem_read);
    sem_post(sem_mutex);  // Release the mutex
}

/**
 * @brief colors the edges in a graph
 * @details colors the edges of a graph using the Monte Carlo randomness algorythm and 
 * gives them on of three available colors that are speicifies as one of the numbers 
 * 0, 1, 2 or RED, BLUE, GREEN
 * @param edges the list of edges that needs to be colored
 * @param limit is just for passin variables
 * @param num_of_edges is the total number of edges that need to be colored
*/
static void colorEdgesMonteCarlo(edge *edges, size_t limit, int num_of_edges) {

    // Set all colors to -1 so we know which ones don't have a color yet
    for (size_t i = 0; i < num_of_edges; i++) {
        edges[i].col = -1;
    }

    // Now color all edges and try to avoid two edges having the same color
    size_t collisions = 0;

    for (size_t i = 0; i < num_of_edges && collisions < limit; i++) {
        edge *edge1 = &edges[i];

        if (edge1->col == -1) {
            edge1->col = rand() % 3;
        }
    }
}

/**
 * @brief calculates one solution to the 3coloring problem
 * @details it checks whether two adjecent edges have the same color and if 
 * they do it adds the vertices connecting the two edges to a list of vertices that need to be deleted
 * @param list is a list of the vertices that need to be 
 * @param edges are all the edges in the graph
 * @param vertices are all the vertices in the graph
 * @param num_of_edges is the number of all the edges
 * @param num_of_verties is the total number of vertices
*/
static void get_one_solution(edge_list **list, edge *edges, vertex *vertices, int num_of_edges, int num_of_vertices) {
    (*list)->num_of_vertices = 0;
    vertex v_to_add;

    for (size_t i = 0; i < num_of_edges; i++) {
        for (size_t j = 1 ; j < num_of_edges; j++) {
            //chekc if such vertex exists
            v_to_add = get_vertex(edges[i], edges[j], vertices, num_of_vertices);
            if (v_to_add.from.number == -1) {
                //it don't, so we just do nothing
            } else {
                //if they have the same color, remove vertex
                if (edges[i].col == edges[j].col) {
                // Assuming list is allocated correctly before this point
                    if ((*list)->num_of_vertices < MAX_NUM_OF_EDGES_IN_LIST) {
                    (*list)->edges[(*list)->num_of_vertices] = v_to_add;
                    (*list)->num_of_vertices++;
                } else {
                // Handle the case where the array is full
                    }

                }
            }
        }
    }
}


int main(int argc, char ** argv){

    prog_name = argv[0];

    struct cirula_buffer *buffer = NULL;

    int num_of_balls = 0;

    int shmfd = 0;

    if (argc <= 1) {
        error_eit("No edges given\n");
    }

    // opening shared memory
    opening_resources(&buffer, &shmfd);

    struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handle_signal;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);

    // get input from user
    vertex *list = malloc(sizeof(vertex) * argc);

    if (list == NULL)
    {
        error_eit("Failed to allocate memory in variable list");
    }
    
    vertex *vertex1 = malloc(sizeof(vertex));

    if (vertex1 == NULL)
    {
        error_eit("Failed to allocate memory in variable vertex1");
    }

    get_input(&list,argc,argv,vertex1);
    free(vertex1);

    //get all balls asap, i repeat , all units, we need the balls
    edge*edges = malloc(sizeof(edge)*argc);

    if (edges == NULL)
    {
        error_eit("Failed to allocate memory in variable edges");
    }

    get_all_balls(&edges,&num_of_balls,list,argc-1);
    //give the balls colors ya know
    struct timeval tv;
	gettimeofday(&tv, NULL);
	srand(tv.tv_usec ^ getpid());

    //make solutions till you get a result

    while (buffer->terminate != 1 && !quit)
    {
        edge_list * one_solution = malloc(sizeof(edge_list));
        
        colorEdgesMonteCarlo(edges, 8, num_of_balls);
        get_one_solution(&one_solution, edges, list, num_of_balls, argc);
        
        //keep writing solutions to the shared memory till limit is reached
        //check for sem_wait conditions
        if (sem_wait(sem_mutex) == -1)
        {
            //ccheck if interupted by a signal
            if (errno == EINTR)
            {
                continue;
            }

        error_eit("Fail while decrementing sem_mutex");
        }

        if (sem_wait(sem_write) == -1)
        {
            //ccheck if interupted by a signal
            if (errno == EINTR)
            {
                continue;
            }

            error_eit("Fail while decrementing sem_mutex");
        }


        write_buffer(one_solution, buffer);
        
        buffer->limit ++;   

        free(one_solution);  

        if (buffer->terminate == 1)
        {
            break;
        }
    }


    (*buffer).terminate = 1;

    //erase edges with the same colors that are connected to make it three collerable

    //generate a bunch of solutions till its three collerable

    // clean up of resources
    free(list);
    free(edges);
    clean_up(buffer);

    exit(EXIT_SUCCESS);
}
