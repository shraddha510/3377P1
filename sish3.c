#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_LINE 1024
#define MAX_ARGS 64
#define MAX_HISTORY 100
#define HISTORY_SIZE 100
#define MAX_COMMAND_COUNT 100

char history[HISTORY_SIZE][MAX_COMMAND_COUNT];
int history_index = 0;
int command_count = 0;

int general_execute(char **args);
void err_exit(char *msg);

int history_count = 0;

void tokenize_input(char *input, char **args)
{
    int i = 0;
    args[i] = strtok(input, " ");
    while (args[i] != NULL)
    {
        args[++i] = strtok(NULL, " ");
    }
    /*
    //debugging
    Print the contents of args
    printf("tokens:\n");
    for (int j = 0; j < i; j++)
    {
        printf("args[%d]: %s\n", j, args[j]);
    }*/
}

void history_add(const char *command)
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

void history_c()
{
    // Clear the history
    history_index = 0;
    command_count = 0;
}

void history_print()
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

int general_execute(char **args)
{
    if (args[0] == NULL)
    {
        return 1;
    }

    if (strcmp(args[0], "exit") == 0)
    {
        return 0;
    }
    else if (strcmp(args[0], "cd") == 0)
    {
        if (args[1])
        {
            if (chdir(args[1]) != 0)
            {
                perror("sish");
            }
        }
        else
        {
            fprintf(stderr, "sish: expected argument to \"cd\"\n");
        }
    }
    else if (strcmp(args[0], "history") == 0)
    {
        if (args[1] && strcmp(args[1], "-c") == 0)
        {
            history_c();
        }
        else if (args[1])
        {
            history_offset(atoi(args[1]));
        }
        else
        {
            history_print();
        }
    }
    else
    {
        pid_t pid = fork();
        if (pid == 0)
        {
            execvp(args[0], args);
            perror("sish");
            exit(EXIT_FAILURE);
        }
        else if (pid > 0)
        {
            wait(NULL);
        }
        else
        {
            perror("sish");
        }
    }
    return 1;
}

void piped_commands(char *input)
{
    // Declarations
    int num_commands = 0;
    char *commands[MAX_ARGS];
    pid_t child_pids[MAX_ARGS];

    // Tokenize user input into individual commands
    commands[num_commands] = strtok(input, "|");
    while (commands[num_commands] != NULL && num_commands < MAX_ARGS - 1)
    {
        num_commands++;
        commands[num_commands] = strtok(NULL, "|");
    }

    // Create pipes
    int pipes[num_commands - 1][2];
    int i;
    for (i = 0; i < num_commands - 1; i++)
    {
        if (pipe(pipes[i]) == -1)
            err_exit("pipe");
    }

    // Fork child processes
    for (i = 0; i < num_commands; i++)
    {
        child_pids[i] = fork();
        if (child_pids[i] < 0)
            err_exit("fork failed");

        if (child_pids[i] == 0)
        {
            // Child process
            if (i > 0)
            {
                // Set up input redirection for child processes (except the first one)
                if (dup2(pipes[i - 1][0], STDIN_FILENO) == -1)
                    err_exit("dup2 failed for stdin");
                close(pipes[i - 1][0]); // Close read end of previous pipe
                close(pipes[i - 1][1]); // Close write end of previous pipe
            }

            if (i < num_commands - 1)
            {
                // Set up output redirection for child processes (except the last one)
                if (dup2(pipes[i][1], STDOUT_FILENO) == -1)
                    err_exit("dup2 failed for stdout");
                close(pipes[i][0]); // Close read end of current pipe
                close(pipes[i][1]); // Close write end of current pipe
            }

            // Execute command
            char *args[MAX_ARGS];
            tokenize_input(commands[i], args);
            execvp(args[0], args);
            // If execvp fails
            err_exit("execvp failed");
        }
        else
        {
            // Parent process
            if (i > 0)
            {
                // Close unused read end of previous pipe
                close(pipes[i - 1][0]);
                close(pipes[i - 1][1]);
            }
        }
    }

    // Close all pipe ends in parent process
    for (i = 0; i < num_commands - 1; i++)
    {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // Wait for all child processes to finish
    for (i = 0; i < num_commands; i++)
    {
        waitpid(child_pids[i], NULL, 0);
    }
}

void initial(char *input)
{
    char *args[MAX_ARGS];

    if (strchr(input, '|'))
    {
        piped_commands(input);
    }
    else
    {
        tokenize_input(input, args);
        general_execute(args);
    }
}

int main()
{
    char *user_input = NULL;
    size_t input_size = 0;
    char *input_copy = NULL; // Variable to store a copy of the user input

    while (1)
    {
        printf("sish> ");
        fflush(stdout);

        // printf("before %s\n", user_input);
        ssize_t read = getline(&user_input, &input_size, stdin);
        // printf("before %zd\n", read);

        if (read == -1)
        {
            perror("getline");
            exit(EXIT_FAILURE);
        }

        if (read > 0 && user_input[read - 1] == '\n')
        {
            user_input[read - 1] = '\0';
        }

        // Allocate memory for input_copy and copy user_input to it
        input_copy = strdup(user_input);

        // printf("before %s\n", user_input); //debug
        initial(user_input); // execute command prior to adding to history
        // printf("after %s\n", user_input); //debug
        history_add(input_copy); // Add input_copy to history
        command_count++;

        // Free memory allocated for input_copy
        free(input_copy);
        input_copy = NULL; // Reset input_copy to NULL
    }

    free(user_input);
    return 0;
}

void err_exit(char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}
