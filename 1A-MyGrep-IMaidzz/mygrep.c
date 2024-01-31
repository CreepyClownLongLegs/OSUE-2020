/**
 * @file mygrep.c
 * @author [Vorname] [Nachname] <matrikelnummer@student.tuwien.ac.at>
 * @date 13.11.2023
 *
 * @brief Program find a given keyword in the input that is given to it either via stdin 
 * or from input file(s), afterwards prints the lines that contain the keyword to either stdout
 * or a specified output file
 **/

#include <ctype.h>
#include <getopt.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

/**
 * the name of the program
 * needed later also for the execlp function
*/

char *prog_name;

/**
 * @brief
 * gives short user manuel of the program
 * 
 * @details
 * The function is used to explain the usage of the program 
 * whenever it seems like the user is confused on how to use the program
*/

void usage(void) {
    fprintf(stdout, "USAGE: %s [-i] [-o outfile] keyword [file...]\n",prog_name);
    fprintf(stdout, "[-i] : options argument for case insesitivity\n");
    fprintf(stdout, "[-o output] : options argument that takes a output file\n");
    fprintf(stdout, "keyword : the word that needs to be searched for\n");
    fprintf(stdout, "[file...] : input files, if no specified program reads from stdin\n");
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

void error(const char *msg) {
    fprintf(stderr, "%s: %s\n", prog_name, msg);
    exit(EXIT_FAILURE);
}

/**
 * @brief
 * gives us a word in upper case
 * 
 * @details
 * We give the functions two strings an input and output strings,
 * the first one is left unhanged and in the second one we save the 
 * first word with all upper case letters
 * 
 * @param input the input word which needs to get converted
 * @param output the destination word at which we save the converted first word
*/

void word_to_upper(const char *input, char **output) {

    size_t len = strlen(input);
    *output = malloc((len + 1) * sizeof(char));
    if (*output == NULL) {
        perror("Failed to allocate memory");
        exit(EXIT_FAILURE);
    }

    for (size_t i = 0; i < len; i++) {
        // Convert alphabetic characters and specified symbols to uppercase
        if (isalnum(input[i]) || strchr(".,:-!=?%", input[i]) != NULL) {
            (*output)[i] = toupper(input[i]);
        } else {
            // Preserve other characters unchanged
            (*output)[i] = input[i];
        }
    }
    (*output)[len] = '\0';
}

int main(int argc, char **argv) {

    prog_name = argv[0];
    //counts occuranes of the o options
    int o = 0;
    //counts occurances of the i option
    int i = 0;
    //takes positional arguments of the names of input files
    char **input_files = NULL;
    //otherwise the input is stdin
    FILE *input = stdin;
    //takes the name of the output file
    char *output_file = NULL;
    //otherwise the output is stdout
    FILE *output = stdout;
    //saves the number count of input files
    int number_of_input_files = 0;
    int c;
    //saves keyword
    char *keyword = NULL;

    /**
     * we allocate memory for all the input files to be processed
    */

    input_files = (char **)malloc(argc * sizeof(char *));
    if (input_files == NULL) {
        perror("Failed to allocate memory for input_files");
        exit(EXIT_FAILURE);
    }

    /**
     * we process the option arguments given and the positional arguments
    */
    while ((c = getopt(argc, argv, "o:i")) != -1) {
        switch (c) {
        case 'o':
            if (optarg != NULL) {
                if (o > 0) {
                    error("Option argument -o given more than once");
                    usage();
                }
                output_file = strdup(optarg);
                if (output_file == NULL) {
                    perror("Failed to allocate memory for output_file");
                    usage();
                    exit(EXIT_FAILURE);
                }
                o++;
            } else {
                usage();
                error("Option -o requires an argument");
            }
            break;

        case 'i':
            i++;
            if (i > 1) {
                usage();
                error("Option argument -i given more than once");
            }
            break;

        case '?':
            usage();
            error("Invalid argument given");

        default:
            break;
        }
    }

    /**
     * if no keyword is given we terminate the program and inform the user
    */
    if (optind == argc) {
        usage();
        error("You must give a keyword");
    }

    /**
     * we save the keyword  
    */

    keyword = strdup(argv[optind]);

    /**
     * we count the number of input files here
    */
    for (size_t i = optind + 1; i < argc; i++) {
        input_files[number_of_input_files] = strdup(argv[i]);
        if (input_files[number_of_input_files] == NULL) {
            perror("Failed to allocate memory for input file name");
            exit(EXIT_FAILURE);
        }
        number_of_input_files++;
    }

    /**
     * we open the output file if one is given
    */
    if (o > 0) {
        output = fopen(output_file, "w");
        if (output == NULL) {
            perror("Failed opening output file");
            free(output_file);
            exit(EXIT_FAILURE);
        }
    }

    char *line = NULL;
    size_t len = 0;
    char *uppered_line = NULL;
    char *uppered_keyword = NULL;

    for (size_t j = 0; j < number_of_input_files; j++) {
        /**
         * we open the input files one by one
        */
        input = fopen(input_files[j], "r");
        if (input == NULL) {
            perror("Failed opening input file");
            exit(EXIT_FAILURE);
        }

        /**
         * we treat the input lines differently depending on if they are case insensitive or not
        */
        if (i>0)
        {
            //case insensitive

            while (getline(&line, &len, input) != -1) {
            
            uppered_line = NULL;
            uppered_keyword = NULL;

            /**
             * we first convert all the lines and the keyword to be uppercase letters and then do the comparisons
            */

            word_to_upper(line, &uppered_line);
            word_to_upper(keyword, &uppered_keyword);

            if (strstr(uppered_line, uppered_keyword) != NULL) {
                fprintf(output, "%s", line);
            }

            //we free the uppercase input  and  uppercase keyword everytime

            free(uppered_line);
            free(uppered_keyword);
            }
        }
        else{
            /**
             * case sensitive
             * the comparions is done with the lines as they are
            */
            while (getline(&line, &len, input) != -1) {
                if (strstr(line, keyword) != NULL) {
                fprintf(output, "%s", line);
            }
            }
        }
        fclose(input);
    }

    /**
     * in case we did not have any input files
    */
    if (number_of_input_files < 1)
    {
        /**
         * we take input from stdin and otherwise the comparions are done very much he same
         * way the are done when there are input files
        */
        if (i>0)
        {
            while (getline(&line, &len, input) != -1) {
            
            uppered_line = NULL;
            uppered_keyword = NULL;

            word_to_upper(line, &uppered_line);
            word_to_upper(keyword, &uppered_keyword);

            if (strstr(uppered_line, uppered_keyword) != NULL) {
                fprintf(output, "%s", line);
            }

            free(uppered_line);
            free(uppered_keyword);
            }
        }
        else{
            while (getline(&line, &len, input) != -1) {
                if (strstr(line, keyword) != NULL) {
                fprintf(output, "%s", line);
            }
            }
        }
    }
    
    /**
     * we free all the resources
    */

    free(line);
    for (size_t i = 0; i < number_of_input_files; i++) {
        free(input_files[i]);
    }
    free(input_files);
    free(output_file);
    free(keyword);

    exit(EXIT_SUCCESS);
}
