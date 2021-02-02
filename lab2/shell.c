#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#define MAX_LINE 80 /* 80 chars per line, per command */
#define MAX_ARGS (MAX_LINE / 2 + 1) /* command line (of 80) has max of 40 arguments */
#define DELIMITER " \n"

void init_args(char *args[]);
void refresh_args(char *args[]);
void init_command(char *command);
int get_input(char *command);
int parse_input(char *args[], char *original_command);

int main(void) {
	char *args[MAX_ARGS];
	char command[MAX_LINE];
	int should_run = 1;

	init_args(args);
	init_command(command);
	while (should_run) {
		printf("osh>");
		fflush(stdout);
		fflush(stdin);

		refresh_args(args);

		if(!get_input(command)) continue;

		int args_num = parse_input(args, command);

        int i;
        for (i = 0; i < args_num; i++) {
            printf("arg[%d]: %s\n", i, args[i]);
        }

		/**
		 * After reading user input, the steps are:
		 * (1) fork a child process
		 * (2) the child process will invoke execvp()
		 * (3) if command included &, parent will invoke wait()
		 */
	}

	return 0;
}

void init_args(char *args[]) {
	int i;
    for(i = 0; i < MAX_ARGS; i++)
        args[i] = NULL;
}

void refresh_args(char *args[]) {
    while(*args) {
        free(*args);
        *args++ = NULL;
    }
}

void init_command(char *command) {
    strcpy(command, "");
}

int get_input(char *command) {
    char input_buffer[MAX_LINE];
    fgets(input_buffer, MAX_LINE, stdin);
	
    if(strncmp(input_buffer, "!!", 2) == 0) {
        if(strlen(command) == 0) {
            fprintf(stderr, "No commands in history.\n");
            return 0;
        }
        printf("%s", command);
        return 1;
    }
    strcpy(command, input_buffer);
    return 1;
}

int parse_input(char *args[], char *original_command) {
    char command[MAX_LINE];
    strcpy(command, original_command);
    char *token = strtok(command, DELIMITER);

    int i;
    for (i = 0; token != NULL; i++) {
        args[i] = malloc(strlen(token) + 1);
        strcpy(args[i], token);
        token = strtok(NULL, DELIMITER);
    }
    return i;
}