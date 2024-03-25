#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_INPUT_LENGTH 1024
#define MAX_ARGS 300
#define HISTORY_SIZE 100
#define MAX_COMMAND_COUNT 100

char history[HISTORY_SIZE][MAX_COMMAND_COUNT];
int history_index = 0;
int command_count = 0;

void general_execute(char *args[]);
void add_to_history(const char *command);
void clear_history();
void history_offset(int offset);
void print_history();
void piped_commands(char *commands[], int pipe_count);
void err_exit(char *msg);

int main()
{
    char *input = NULL;
    size_t input_len = 0;
    char *args[MAX_ARGS];
    char *token;
    int arg_count = 0;

    while (1)
    {
        printf("sish> ");

        // Read input using getline
        if (getline(&input, &input_len, stdin) == -1)
        {
            printf("Error reading input\n");
            continue;
        }

        // Remove newline character
        input[strcspn(input, "\n")] = '\0';

        // Add command to history
        add_to_history(input);
        command_count++;

        // Tokenize input using strtok_r
        char *saveptr;
        token = strtok_r(input, " ", &saveptr);
        while (token != NULL)
        {
            args[arg_count++] = token;
            if (arg_count >= MAX_ARGS)
            {
                printf("Too many arguments\n");
                break;
            }
            token = strtok_r(NULL, " ", &saveptr);
        }
        args[arg_count] = NULL;

        if (arg_count > 0)
        {
            general_execute(args);
        }

        arg_count = 0;
    }

    // Free dynamically allocated memory
    free(input);

    return 0;
}

void general_execute(char *args[])
{
    // Check if the command is a pipe command
    int i, pipe_count = 0;
    char *commands[MAX_COMMAND_COUNT]; // Array to store individual commands

    for (i = 0; args[i] != NULL; i++)
    {
        if (strcmp(args[i], "|") == 0)
        {
            pipe_count++;
        }
    }

    // If there are no pipes, execute the command as usual
    if (pipe_count == 0)
    {
        // Check if the command is a built-in command
        if (strcmp(args[0], "exit") == 0)
        {
            // Exit the shell
            exit(EXIT_SUCCESS);
        }
        else if (strcmp(args[0], "cd") == 0)
        {
            // Change directory
            if (args[1] == NULL)
            {
                printf("cd: missing argument\n");
            }
            else
            {
                if (chdir(args[1]) != 0)
                {
                    perror("cd");
                }
            }
        }
        else if (strcmp(args[0], "history") == 0)
        {
            // Print or execute from history
            if (args[1] == NULL)
            {
                print_history();
            }
            else if (strcmp(args[1], "-c") == 0)
            {
                clear_history();
            }
            else
            {
                int offset = atoi(args[1]);
                history_offset(offset);
            }
        }
        else
        {
            // Execute external command
            pid_t pid = fork();
            if (pid == 0)
            {
                // Child process
                execvp(args[0], args);
                perror("execvp");
                exit(EXIT_FAILURE);
            }
            else if (pid < 0)
            {
                // Fork failed
                perror("fork");
            }
            else
            {
                // Parent process
                wait(NULL);
            }
        }
    }
    else
    {
        // If there are pipes, handle pipe commands
        piped_commands(commands, pipe_count);
    }
}

void piped_commands(char *commands[], int pipe_count)
{
}

void add_to_history(const char *command)
{
    if (command_count < HISTORY_SIZE)
    {
        history_index = (command_count);
        strcpy(history[command_count], command);
    }
    else
    {
        history_index = (command_count) % HISTORY_SIZE;
        strcpy(history[history_index], command);
    }
}

void clear_history()
{
    // Clear the history
    history_index = 0;
    command_count = 0;
}

void print_history()
{
    int i, j;

    for (i = 0, j = 0; i < command_count; j++)
    {
        printf("%d %s\n", j, history[i]);
        i++;
    }
}

void history_offset(int offset)
{
    if (offset >= 0 && offset < command_count)
    {
        char *cmd = history[offset];
        char *args[MAX_ARGS];
        char *token;
        int arg_count = 0;

        token = strtok(cmd, " ");
        while (token != NULL)
        {
            args[arg_count++] = token;
            if (arg_count >= MAX_ARGS)
            {
                printf("Too many arguments\n");
                break;
            }
            token = strtok(NULL, " ");
        }
        args[arg_count] = NULL;

        general_execute(args);
    }
    else
    {
        printf("Invalid offset\n");
    }
}

void err_exit(char *msg)
{
    perror(msg);
}

