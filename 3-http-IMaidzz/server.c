/**
 * @file server.c
 * @author Maida Horozovic <e11927096@student.tuwien.ac.at>
 * @date 15.01.2023
 * 
 * @brief The server program.
 * 
 * Implementation of a simple HTTP 1.1 server
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <time.h>
#include <fcntl.h>
#include <getopt.h>

//buffer size used when reading and copying files from a stream
#define BUFFER_SIZE 1024

//name of program primarilly used for error messages
char * prog_name = NULL;

//atomatic value of a signal occurance
volatile sig_atomic_t alive = true;

/**
 * @brief A simple signal handler.
 * 
 * @details Depending on whather a signal occurs and what signal occurs,
 * the global variable alive shall be changed to false
 * 
 * @param signal The signal to handle.
 */
static void handle_signal(int signal)
{
    alive = false;
}

/**
 * @brief
 * gives an error message
 * 
 * @details
 * Specifies the exact problem that occured in the program with 
 * and error message and terminates program with EXIT_FAILURE
 * 
 * @param msg the error message we convey to the user
*/
static void error_exit(char * msg){
    fprintf(stderr,"%s: %s\n", prog_name, msg);
    exit(EXIT_FAILURE);
}


/**
 * @brief Informs the user of the correct usage of the program
 * @details If a error occurs that has to do with the input of the user,
 * this error message is displayed to inform the user of the correct usage of the program.
 */
static void usage(void){
    fprintf(stderr, "USAGE : [%s] server [-p PORT] [-i INDEX] DOC_ROOT\n", prog_name);
}

/**
 * @brief Making a new socket
 * 
 * @details Creates a new socket and in case an error occurs during the 
 * process of creation, terminates program.
 * 
 * @param port The port at which the socket should be created.
 * 
 * @return The file descriptor of the created socket is returned
 */
static int make_socket( char * port){
    struct addrinfo hints, *ai;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int res = getaddrinfo(NULL, port, &hints, &ai);
    if (res != 0)
    {
        error_exit("Error while creating adress");
    }
    
    int socket_fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (socket_fd == -1)
    {
        close(socket_fd);
        error_exit("Error while making soket occured");
    }

    int optval = 1;
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);

    if (bind(socket_fd, ai->ai_addr, ai->ai_addrlen) < 0)
    {
        close(socket_fd);
        error_exit("Error while binding sockets");
    }
    
    if (listen(socket_fd,32) < 0)
    {
        close(socket_fd);
        error_exit("Error while invoking listen");
    }
    
    freeaddrinfo(ai);
    return socket_fd; 
}

/**
 * @brief Writes a HTTP/1.1 error header to a socket
 * 
 * @details We are using dprintf to avoid using fflush and avoid errors associated with it
 * 
 * @param error the exact error which needs to be sent, like 404 file not found.
 * @param socket the socket we need to send the message thru
 */
static void send_error_response(int socket, char * error){
    dprintf(socket, "HTTP/1.1 %s\r\nConnection: close\r\n\r\n", error);
}

/**
 * @brief Writes a HTTP/1.1 succsses header to a socket
 * 
 * @details We are using dprintf to avoid using fflush and avoid errors associated with it
 * The message is comprimised of the HTTP 200 OK message, the date, content length and a Connection:close message
 * 
 * @param content_length it is the content length of the http file that needs to be sent
 * @param socket the socket we need to send the message thru
 */
static void send_valid_response(int socket, size_t content_lenght){

        // Find out the time
    char time_text[200];
    time_t t = time(NULL);
    struct tm *tmp;
    tmp = gmtime(&t);
    strftime(time_text, sizeof(time_text), "%a, %d %b %y %T %Z", tmp);
    dprintf(socket, "HTTP/1.1 200 OK\r\nDate: %s\r\nContent-Length:%ld\r\nConnection:close\r\n\r\n", time_text, content_lenght);
}


/**
 * @brief Handle incomming requests from clients
 * @details Checks the certain header criteria that need to be met
 * by the request header, furthermore comprimises the message it then needs to send
 * back in case a valid file was requested with a valid method like GET and the HTTP 
 * protocol was correct, otherwise program sends an error message
 * @param socket_fd its actually the file descriptor of the accepted connection
 * @param request_file The name of the requested file
 * @param doc_root The name of the firectory in the server that holds all http files that
 * could possibly be accessed by the clients
 * @return 
 */
static void read_request(int socket_fd, char *root_doc, char *request_file)
{
    FILE *socket_file = fdopen(socket_fd, "r+");

    if (socket_file == NULL)
    {
        close(socket_fd);
        error_exit("Error while trying to open file");
    }

    char *line = NULL;
    size_t size = 0;

    if (getline(&line, &size, socket_file) == -1)
    {
        close(socket_fd);
        fclose(socket_file);
        usage();
        error_exit("Failed getting the first header line of the request");
    }

    char *req_method = strtok(line, " ");
    char *path_of_file = strtok(NULL, " ");
    char *protool = strtok(NULL, "\r");

    if (req_method == NULL )
    {
        send_error_response(socket_fd, "400 (Bad Request)");
        close(socket_fd);
        fclose(socket_file);
        return;
    }
    

    if (path_of_file == NULL )
    {
        send_error_response(socket_fd, "400 (Bad Request)");
        close(socket_fd);
        fclose(socket_file);
        return;
    }

    if ((protool == NULL || (strcmp(protool, "HTTP/1.1") != 0)))
    {

        send_error_response(socket_fd, "400 (Bad Request)");
        close(socket_fd);
        fclose(socket_file);
        return;
    }

    if ((strcmp(req_method, "GET") != 0) )
    {
        send_error_response(socket_fd, "501 (Not implemented)");
        close(socket_fd);
        fclose(socket_file);
        return;
    }

    // Make file name
    char file[1024]; // Adjust the size according to your needs
    snprintf(file, sizeof(file), "%s%s%s", root_doc, path_of_file, (strcmp(path_of_file, "/") == 0) ? request_file : "");

    printf("%s\n", file);

    // Copy so-called file
    FILE *toread = fopen(file, "r");

    // Error handling

    if (toread == NULL)
    {
        // File not found, send a 404 response
        send_error_response(socket_fd, "404 (Not Found)");
        fclose(socket_file);
        close(socket_fd);
        free(line);
        return;
    }
            // Get file size
            fseek(toread, 0, SEEK_END);
            size_t filesize = ftell(toread);
            fseek(toread, 0, SEEK_SET);

            // Send valid response
            send_valid_response(socket_fd, filesize);

            // Read and send file content
            char buffer[BUFFER_SIZE];
            size_t bytesRead;
            while ((bytesRead = fread(buffer, 1, sizeof(buffer), toread)) > 0)
            {
                write(socket_fd, buffer, bytesRead);
            }

    fclose(toread);
    fclose(socket_file);
    free(line);
}


int main(int argc, char ** argv){

prog_name = argv[0];

// Setup the singal handler
struct sigaction sa;
memset(&sa, 0, sizeof(sa));
sa.sa_handler = handle_signal;
sigaction(SIGINT, &sa, NULL);
sigaction(SIGTERM, &sa, NULL);

char * port_num = NULL;
int p_option = 0;
char * doc_root = NULL;
char * file_name = NULL;
int file_option = 0;

int c = 0;

//argument handling
while ((c = getopt(argc, argv, "p:i:")) != -1)
{
    switch (c)
    {
    case 'p':

        if (p_option > 0)
        {
            usage();
            error_exit("Port was already given once");
        }

        port_num = optarg;
        p_option ++;
        break;
    
    case 'i':
        if (file_option > 0)
        {
            usage();
            error_exit("File was already given once");
        }

        file_name = optarg;
        file_option ++;
        break;
    
    case '?':
        usage();
        error_exit("Invalid option given");
        break;
    
    default:
        break;
    }
}

if (argc == optind)
{
    error_exit("No DOC ROOT given");
}
else{
    doc_root = argv[optind];
}

if (p_option == 0)
{
    port_num = "8080";
}

if (file_option == 0)
{
    file_name = "index.html";
}

//all arguments set

//opening and setting socket
int sock_fd = make_socket(port_num);

//read from socket

while (alive)
{
    int confd = accept(sock_fd, NULL, NULL);

    if (confd == -1)
    {
        if (errno != EINTR)
        {
            usage();
            close(sock_fd);
            fprintf(stderr, "%s : Unable to connect to socket", prog_name);
        }

        alive = false;
        break;
    }

    printf("connection established\n");
    read_request(confd, doc_root, file_name);
    
}

//close resources and exit
close(sock_fd);
return EXIT_SUCCESS;
}