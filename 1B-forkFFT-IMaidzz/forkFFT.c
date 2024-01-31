/**
 * @file forkFFT.c
 * @author Maida Horozovic <e11927096@student.tuwien.ac.at>
 * @date 13.11.2023
 *
 * @brief The program calculates the FFT value of the given inputs which must be two floating point values per line
 * symbolising the real and imaginary part of the imaginary number, and there must be an even number of imaginary numbers
 * The only optinal arggument -p symply specifies that the result should not print more than 3 deimal values after the dot
 **/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <regex.h>
#include <string.h>
#include <math.h>
#include <complex.h>
#include <errno.h>
#include <ctype.h>

//from the four pipes the one designated for the first child to read from
#define FIRST_CHILD_READ (0)
//from the four pipes the one designated for the first child to write to
#define FIRST_CHILD_WRITE (1)
//from the four pipes the one designated for the second child to read from
#define SECOND_CHILD_READ (2)
//from the four pipes the one designated for the second child to write to
#define SECOMD_CHILD_WRITE (3)

//defines the write end of a pipe
#define WRITE (1)
//defines the read end of a pipe
#define READ (0)

//the mathematical constant Pi
#define PI (3.141592654)

//standin buffersize for memory allocation
#define BUFSIZE (200)

char * prog_name;

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

void error_exit(const char * msg){

    fprintf(stderr, "%s : %s\n",prog_name ,msg);
    exit(EXIT_FAILURE);
}

/**
 * @brief
 * gives short user manuel of the program
 * 
 * @details
 * The function is used to explain the usage of the program 
 * whenever it seems like the user is confused on how to use the program
 * 
 * @param prog_name the name of the program
*/
void usage(char* prog_name){
    fprintf(stdout, "USAGE: %s [-p]\n",prog_name);
    fprintf(stdout, "[-p]: Option argument specifying that the output numbers should only have 3 decimal places, otherwise it's 6\n");
}

/**
 * @brief
 * closes all pipes in current program
 * 
 * @param pipes the pipes that need to be closed
*/

void close_all_pipes(int pipes[4][2]){

    for (size_t i = 0; i < 4; i++)
    {
        close(pipes[i][READ]);
        close(pipes[i][WRITE]);
    }

}

/**
 * @brief
 * Puts the parent process ina  state of waiting for the kids
 * 
 * @details
 * The parents wait till all the 2 child processes given as arguments 
 * terminate and then take their termations status, if a cchild process 
 * terminated with anything but EXIT_SUCCESS , the parent program also terminates
 * 
 * @param pid1 the id of the first child process
 * @param pid2 the id of the second child process
*/

void wait_for_kid(pid_t pid1, pid_t pid2)
{
    int status = 0;
    int status2 = 0;

    while (waitpid(pid1, &status, 0) == -1);

    if (WEXITSTATUS(status) != EXIT_SUCCESS) {
        error_exit("First child exited with EXIT_FAILURE");
    }

    while (waitpid(pid2,&status2,0) == -1);

    if (WEXITSTATUS(status2) != EXIT_SUCCESS)
    {
        error_exit("Second child exited with EXIT_FAILURE");
    }
    
}

/**
 * @brief
 * doubles the capaciry of a buffer
 * 
 * @details doubles the capacity of our input buffer for the
 * imaginary numbers when it reaches its limit
 * 
 * @param buffer buffer that needs to be doubled
 * @param currentSize is the size of the buffer in int
*/

void resizeBuffer(double complex **buffer, int*currentSize) {
    size_t newSize = *currentSize * 2;
    *buffer = (double complex *)realloc(*buffer, newSize * sizeof(double complex));
    if (*buffer == NULL) {
        fprintf(stderr, "Fehler: Speicher konnte nicht neu allokiert werden.\n");
        exit(EXIT_FAILURE);
    }
    *currentSize = newSize;
}

/**
 * @brief corrects -0 values in imaginary numbers
 * 
 * @details takes the imaginary number and checks it for -0 
 * values by checkcing if their absolute values are less than 
 * 1e - 2
 * 
 * @param rez the complex number that is checked for  -0 values
 * @return returns the complex number that needed to be checked
*/

double complex check_for_minus_zeros(double complex rez){

    double complex z = rez;

    if (fabs(creal(rez))< 1e-2)
    {
        z = 0 + cimag(rez)*I;
    }

    if (fabs(cimag(rez))< 1e-2)
    {
        z = 0*I + creal(z);
    }

    return z;
}

/**
 * @brief 
 * counts how many foating point values there are in the input line
 * 
 * @details
 * if there are more than 2 floating point values per line the program terminates 
 * and informs the user of the wrong usage
 * 
 * @param line the input line the needs to be checked
 * @return returns the counted number of floats per line
*/

int count_floats_in_line(char *line) {
    int count = 0;
    char *ptr = line;

    while (*ptr != '\0') {
        // Skip leading spaces
        while (*ptr == ' ') {
            ptr++;
        }

        // Convert the substring to a floating-point number
        strtof(ptr, &ptr);

        // Move to the next character after the converted number
        ptr = strchr(ptr, ' ');

        // If no space is found, break out of the loop
        if (ptr == NULL) {
            break;
        }

        ptr++; // Move past the space
        count++;
    }

    return count;
}


/**
 * @brief checks the line for invalid values
 * 
 * @details checks the line for values other than the endline character, i , *, and numeric values,
 * in case those are found it terminates with exit status 1
 * 
 * @param line the line that needs to be checked 
 * @return returns a positive number if the input is invalid and 0 if it isnt invalid
*/
int contains_alphabetic_chars(const char *line) {
    while (*line != '\0') {
        if (*line != '\n' && *line != 'i' && isalpha(*line)) {
            return 1; // Found an alphabetical letter (excluding '\n')
        }
        line++;
    }
    return 0; // No alphabetical letters found (excluding '\n')
}

/**
 * @brief checks if the input line is blank 
 * 
 * @details in case the input line is blank the program terminates with exist status 1
 * 
 * @param line line that needs to be checked 
 * @return returns 0 if it is not blank and 1 if it is
*/

int is_blank_line(const char *line) {
    size_t len = strlen(line);
    for (size_t i = 0; i < len; i++) {
        if (line[i] != ' ' && line[i] != '\t' && line[i] != '\n' && line[i] != '\r') {
            return 0; // Line contains non-whitespace characters
        }
    }
    return 1; // Line is either blank or contains only whitespace characters
}

/**
 * @brief
 * converts line of string into an imaginary number
 * 
 * @details 
 * takes the two floating point values per line and returns them as compley number,
 * the first one being the real value and the second the imaginary value
 * 
 * @param input take input line of string
 * @return the complex number
 * 
*/


double complex get_imaginary_num_from_line(char *input) {

    if (is_blank_line(input) > 0)
    {
        error_exit("you have not given any input or a blank input");
    }

    int count = count_floats_in_line(input);

    if (count > 1)
    {
        error_exit("Too many floating point numbers in one line");
    }

    if (contains_alphabetic_chars(input) > 0)
    {
        error_exit("has illelgal charaters");
    }
        
    char *imaginary;
    double complex c = 0 + 0 * I;
    float real = strtof(input, &imaginary);
    float imag = strtof(imaginary, NULL);
    c = real + imag * I;

    check_for_minus_zeros(c);

    return c;
}

int main(int argc , char **argv){


int pipes[4][2];
char * input = NULL;
size_t count = 0;
int pid;
int pid2;
prog_name = argv[0];
double complex * complex_num ;
int complex_num_count = 0 ;
int p_option = 0;
int c;
int array_size = BUFSIZE;

complex_num = (double complex *)malloc(BUFSIZE * sizeof(double complex));

//get option argument p

while ((c = getopt(argc, argv,":p")) != -1)
{
    switch (c)
    {
    case 'p':
        p_option++;
        if (p_option > 1)
        {
            usage(prog_name);
            error_exit("too many option -p arguments given");
        }
        break;

    case '?':
        usage(prog_name);
        error_exit("Invalid argument given");
        break;

    default:
        break;
    }

}

//we are getting the input lines either from the parent process or from the user

while (getline(&input, &count, stdin) != -1){

    complex_num[complex_num_count] = get_imaginary_num_from_line(input);
    //counting the input lines
    complex_num_count ++ ;

    if (complex_num_count == (array_size - 1))
    {
        //buffer has reached its limit and needs to be resized
        resizeBuffer(&complex_num, &array_size);
    }

};

free(input);

//if the input is just one compley number that one is just printed out
if (complex_num_count == 1)
{
    complex_num[0] = check_for_minus_zeros(complex_num[0]);
    if (p_option > 0)
    {
        fprintf(stdout, "%.3f %.3f*i\n", creal(complex_num[0]), cimag(complex_num[0]));
    }else{
        fprintf(stdout, "%.6f %.6f*i\n", creal(complex_num[0]), cimag(complex_num[0]));
    }
    free(complex_num);
    exit(EXIT_SUCCESS);
}

//if there are to many numbers in a line
if (argc > 4)
{
    free(complex_num);
    error_exit("too many numbers in one line"); 
}

//if there is no input or the number of inputed complex numbers is odd the program terminates
if ((complex_num_count == 0) || (complex_num_count % 2 != 0))
{
    free(complex_num);
    if (complex_num_count == 0)
    {
           error_exit("no input");
    }

    if (complex_num_count %2 != 0)
    {
           error_exit("uneven input");
    }

}

//creating the neccesarary pipes

if (pipe(pipes[0]) ==-1 || pipe(pipes[1]) ==-1 || pipe(pipes[2]) ==-1 ||pipe(pipes[3]) ==-1)
{
    error_exit("Error while creating pipes");
}


//creating the first child process
if ((pid = fork()) ==-1)
{
    error_exit("error while forking child");
}

if (pid == 0)
{
    //duping the first child pipes to stdin and stdout respectively
    if (dup2(pipes[FIRST_CHILD_WRITE][READ], STDIN_FILENO) == -1)
    {
        error_exit("error while dupping reading pipe of child");
    }

    if (dup2(pipes[FIRST_CHILD_READ][WRITE], STDOUT_FILENO) == -1)
    {
        error_exit("error while dupping writing pipe of child");
    }

    close_all_pipes(pipes);

    //executing the parent program in the child recursively
    execlp(prog_name, prog_name, NULL);

    //this line should not be reached if execlp was succsessful
    error_exit("execlp didnt work properly");
}

//creating the second child process
if ((pid2 = fork()) ==-1)
{
    error_exit("error while forking child");
}

if (pid2 == 0)
{
    if (dup2(pipes[SECOMD_CHILD_WRITE][READ], STDIN_FILENO) == -1)
    {
        error_exit("error while dupping reading pipe of child");
    }

    if (dup2(pipes[SECOND_CHILD_READ][WRITE], STDOUT_FILENO) == -1)
    {
        error_exit("error while dupping writing pipe of child");
    }

    close_all_pipes(pipes);

    execlp(prog_name, prog_name, NULL);

    error_exit("execlp didnt work properly");
}

//cclose all unneccaserry pipes for parents 

close(pipes[FIRST_CHILD_READ][WRITE]);
close(pipes[FIRST_CHILD_WRITE][READ]);
close(pipes[SECOND_CHILD_READ][WRITE]);
close(pipes[SECOMD_CHILD_WRITE][READ]);

//writing to child processes

char string[BUFSIZE];

for (int i = 0; i < complex_num_count; i++)
{
    snprintf(string, sizeof(string), "%.6f %.6fi\n", creal(complex_num[i]), cimag(complex_num[i]));

    if (i %2 == 0)
    {
        if (write(pipes[FIRST_CHILD_WRITE][WRITE], string, strlen(string)) == -1){
            free(complex_num);
            close_all_pipes(pipes);
            error_exit("Smthing wrong with writing to the kid processes");
        };
    }else{
        if (write(pipes[SECOMD_CHILD_WRITE][WRITE], string, strlen(string)) == -1){
            free(complex_num);
            close_all_pipes(pipes);
            error_exit("Smthing wrong with writing to the kid processes");
        };
    }
};

free(complex_num);

//close all writing pipes

close(pipes[SECOMD_CHILD_WRITE][WRITE]);
close(pipes[FIRST_CHILD_WRITE][WRITE]);

//putting the parent process to a halt till children terminate

wait_for_kid(pid,pid2);

FILE * file_first = fdopen(pipes[FIRST_CHILD_READ][READ], "r");
FILE * file_second =fdopen(pipes[SECOND_CHILD_READ][READ], "r");

char * input_first = NULL;
size_t len = 0 ;

char * input_second = NULL;
size_t len2 = 0 ;

double complex * complex_num_child ;

long unsigned int half_num = complex_num_count/2;
complex_num_child = (double complex *)malloc(sizeof(double complex) * half_num);

//getting the input back from the child processes

for (int i = 0; i < (complex_num_count/2); i++)
{

    getline(&input_first, &len, file_first);

    if (input_first == NULL)
    {
        error_exit("No input from the first child");
    }
    

    getline(&input_second, &len2, file_second);

    if (input_first == NULL)
    {
        error_exit("No input from the first child");
    }

    //calculate the FFT finally

    double complex c;
    double complex c2;

    c = get_imaginary_num_from_line(input_first);
    c2 = get_imaginary_num_from_line(input_second);

    double constant = -2*PI/complex_num_count;

    //multiplication formula in complex numbers

    //the middle part
    double complex rez = cos(constant*i) + sin(constant*i)*I;
    //multiplzing with the odd result
    rez = rez*c2;
    double complex rez2 = rez;
    //adding with the even result
    rez = creal(rez) + creal(c) + (cimag(rez) + cimag(c))*I;
    //substracting from the odd result
    rez2 =creal(c) - creal(rez2) + (cimag(c) - cimag(rez2))*I;

    rez = check_for_minus_zeros(rez);
    rez2 = check_for_minus_zeros(rez2);

    if (p_option > 0)
    {
        fprintf(stdout, "%.3f %.3f*i\n", creal(rez), cimag(rez));
    }else{
        fprintf(stdout, "%.6f %.6f*i\n", creal(rez), cimag(rez));
    }
    

    complex_num_child[i] = rez2;
}

//printing the now calculated values further
for (int i = 0; i < (complex_num_count/2); i++)
{
    if (p_option > 0)
    {
    fprintf(stdout, "%.3f %.3f*i\n", creal(complex_num_child[i]), cimag(complex_num_child[i]));
    }else{
    fprintf(stdout, "%.6f %.6f*i\n", creal(complex_num_child[i]), cimag(complex_num_child[i]));
    }
}

//closing the rest of the resources

free(complex_num_child);
free(input_first);
fclose(file_first);
fclose(file_second);

exit(EXIT_SUCCESS);

}