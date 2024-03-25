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

int history_count = 0;

void tokenize_input(char *user_input, char **args)
{
    int i = 0;
    args[i] = strtok(user_input, " ");
    while (args[i] != NULL)
    {
        args[++i] = strtok(NULL, " ");
    }
    /* Print the contents of args
    printf("Tokens:\n");
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

void piped_commands(char *user_input)
{
    char *commands[MAX_ARGS];
    int i = 0;
    commands[i] = strtok(user_input, "|");
    while (commands[i] != NULL)
    {
        commands[++i] = strtok(NULL, "|");
    }

    int num_commands = i;
    int fd[2];
    int fd_input = 0;

    for (i = 0; i < num_commands; i++)
    {
        char *args[MAX_ARGS];
        tokenize_input(commands[i], args);

        pipe(fd);
        pid_t pid = fork();

        if (pid == 0)
        {
            dup2(fd_input, 0); // Change the input according to the old one
            if (i < num_commands - 1)
            {
                dup2(fd[1], 1);
            }
            close(fd[0]);
            execvp(args[0], args);
            perror("sish");
            exit(EXIT_FAILURE);
        }
        else if (pid > 0)
        {
            wait(NULL);
            close(fd[1]);
            fd_input = fd[0]; // Save the input for the next command
        }
        else
        {
            perror("sish");
        }
    }
}

void initial(char *user_input)
{
    char *args[MAX_ARGS];

    if (strchr(user_input, '|'))
    {
        piped_commands(user_input);
    }
    else
    {
        tokenize_input(user_input, args);
        if (!general_execute(args))
            exit(EXIT_SUCCESS);
    }
}

int main()
{
    char *user_input = NULL;
    size_t input_size = 0;

    while (1)
    {
        printf("sish> ");
        fflush(stdout);

        ssize_t read = getline(&user_input, &input_size, stdin);

        if (read == -1)
        {
            perror("getline");
            exit(EXIT_FAILURE);
        }

        if (read > 0 && user_input[read - 1] == '\n')
        {
            user_input[read - 1] = '\0';
        }

        history_add(user_input);
        command_count++;
        initial(user_input);
    }

    free(user_input);
    return 0;
}
