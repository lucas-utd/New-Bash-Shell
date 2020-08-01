// Program:
//        This program is used for creating shsh shell.
// History: 2020/02/26
// Author:  Tao Chen
// Version: 1.0
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/wait.h>

#define READ 0
#define WRITE 1

#define MAXLINE 1024
#define COMMAND_LENGTH 10  /* the max length of each command */
#define MAX_NUM_COMMAND 10 /* the max commands in a pipe shell */
#define MAXFILENAME 128
#define MAXUSER 10         /* the max number useres of UM support, don't use the user zero */

extern int pipePToC[MAXUSER][2];  // the pipes which use for send messages from UM to every user's shsh 
extern int pipeCToP[MAXUSER][2];  // the pipes which use for send messages from every user's shsh to UM


void cmd_function(char *, int userid);
void pipe_function(char *, int userid);

/* execute the commands which are split by ";" */
void shshcmd_function(int userid)
{

    char cmdlines[MAXLINE];
    char command[COMMAND_LENGTH];  /* the input command, such as "cmd", "pipe", "exit" */
    char remain_cmdlines[MAXLINE]; /* the remain of cmdlines which deletes the command */

    while (1)
    {

        for (int i = 0; i < MAXLINE; i++)
            cmdlines[i] = '\0';
        
        if (read(pipePToC[userid][READ], cmdlines, MAXLINE) <= 0)
            exit(0);

        /* replace newline with null */
        if (cmdlines[strlen(cmdlines) - 1] == '\n')
            cmdlines[strlen(cmdlines) - 1] = '\0'; 

        /* getting the command (cmd, pipe, exit) */
        int i = 0;
        int j = 0;
        while ((command[i] = cmdlines[j]) != ' ')
        {
            i++;
            j++;
        }
        command[i] = '\0'; /* replace the end char with '\0' */

        /* skipping the space of the front remaining cmdlines */
        while (cmdlines[j] != '\0' && cmdlines[j] == ' ') 
            j++;

        /* getting the remain of cmdlines */
        i = 0;
        while (cmdlines[j] != '\0')
        {
            remain_cmdlines[i++] = cmdlines[j++];
        }
        remain_cmdlines[i] = '\0'; /* replace the end char with '\0' */

        /* calling the corresponding function */
        if (atoi(command) == userid)
        {   /* chdir to the Home dir which is send from UM */
            chdir(remain_cmdlines);
        }
        else if (strcmp(command, "cmd") == 0)
        {
            printf("Shsh %d processing %s %s\n", getpid(), command, remain_cmdlines);
            fflush(stdout);

            cmd_function(remain_cmdlines, userid);
        }            
        else if (strcmp(command, "pipe") == 0)
        {
            printf("Shsh %d processing %s %s\n", getpid(), command, remain_cmdlines);
            fflush(stdout);

            pipe_function(remain_cmdlines, userid);
            write(pipeCToP[userid][WRITE], "cmd>\n", sizeof("cmd>\n"));
        }           
        else if (strcmp(command, "exit") == 0)
        {
            printf("Shsh %d processing %s\n", getpid(), command);
            printf("Shsh process (userid = %d, pid = %d) terminates.\n", userid, getpid());
            fflush(stdout);

            close(pipePToC[userid][READ]);
            close(pipeCToP[userid][WRITE]);

            _exit(0);
        }
    }
}

/* execute the commands which are split by ";" */
void cmd_function(char *cmdlines, int userid)
{
    char delim[] = ";";
    char *token;
    char *commands[MAX_NUM_COMMAND]; /* the commands in the shell */
    int num_command = 0;             /* the number of commands */

    /* get the commands in shell */
    do
    {
        token = strsep(&cmdlines, delim);
        if (token && num_command < MAX_NUM_COMMAND)
            commands[num_command++] = token;
    } while (token && num_command < MAX_NUM_COMMAND);

    /* save the stdout file descriptor. */
    int save_out = dup(fileno(stdout));

    if (pipeCToP[userid][WRITE] != STDOUT_FILENO);
        dup2(pipeCToP[userid][WRITE], STDOUT_FILENO);

    int rv;
    if (num_command == 1)
    {   
        /* save the stderr file descriptor. */
        int save_err = dup(STDERR_FILENO);
        /* don't show the ERROR messages to monitor */
        dup2(open("/dev/null", O_WRONLY), STDERR_FILENO);

        rv = system(commands[0]);

        /* restore the stderr file descriptor */
        dup2(save_err,STDERR_FILENO);
        close(save_err);

        if (rv != 0)
        {   /* execute one command fail, so the command needs input */
            char back[MAXLINE];
            /* tell the UM this command needs input */
            sprintf(back, "NEEDPRE %s\ncmd>\n", commands[0]);
            write(STDOUT_FILENO, back, sizeof(back));
        } 
        else
        {
            write(STDOUT_FILENO, "cmd>\n", sizeof("cmd>\n"));
        }   
    }
    else if (num_command > 1)
    {
        for (int i = 0; i < num_command; i++)
            system(commands[i]);
        
        write(STDOUT_FILENO, "cmd>\n", sizeof("cmd>\n"));
    }

    /* restore the stdout file descriptor */
    dup2(save_out, fileno(stdout));
    close(save_out);
}

/* pipe the output of parent process to child process */
void pipe_function(char *cmdlines, int userid)
{
    char delim[] = ";";              /* the split character */
    char *token;                     /* the split shell command */
    char *commands[MAX_NUM_COMMAND]; /* the commands in the shell */
    int num_command = 0;             /* the number of commands */

    /* get the commands in shell */
    do
    {
        token = strsep(&cmdlines, delim);
        if (token && num_command < MAX_NUM_COMMAND)
            commands[num_command++] = token;
    } while (token && num_command < MAX_NUM_COMMAND);

    pid_t pid_all[num_command];            /* save the pid of child process */
    int pipearray_all[num_command - 1][2]; /* the pipes array */

    if (num_command == 1) /* one command */
    {
        fprintf(stderr, "The number commands for the pipe is 1. \nPlease input two or more commands in pipe.\n");
        fflush(stderr);
    }
    else if (num_command > 1) /* above two commands */
    {

        if (pipe(pipearray_all[0]) < 0)
        {
            perror("pipe error\n");
            exit(EXIT_FAILURE);
        }

        if ((pid_all[0] = fork()) < 0)
        {
            perror("fork error\n");
            exit(EXIT_FAILURE);
        }
        else if (pid_all[0] == 0) /* the first child process */
        {
            if (dup2(pipearray_all[0][WRITE], STDOUT_FILENO) == -1)
                perror("dup2\n");

            close(pipearray_all[0][READ]);
            close(pipearray_all[0][WRITE]);
            execl("/bin/sh", "sh", "-c", commands[0], (char *)0);
            _exit(127); /* execl error */
        }
        else if (pid_all[0] > 0)
        { /* parent process */
            printf("Shsh pipe processing %s\n", commands[0]);
            printf("Shsh forked %d for shcmd, with in-pipe %d and out-pipe %d\n",
                   pid_all[0], STDIN_FILENO, pipearray_all[0][WRITE]);
        }

        for (int i = 1; i < num_command - 1; i++)
        {
            if (pipe(pipearray_all[i]) < 0)
            {
                perror("pipe error\n");
                exit(EXIT_FAILURE);
            }

            if ((pid_all[i] = fork()) < 0)
            {
                perror("fork error\n");
                exit(EXIT_FAILURE);
            }
            else if (pid_all[i] == 0) /* the middle of command */
            {
                if (dup2(pipearray_all[i - 1][READ], STDIN_FILENO) == -1)
                    perror("dup2 error\n");
                if (dup2(pipearray_all[i][WRITE], STDOUT_FILENO) == -1)
                    perror("dup2 error\n");
                close(pipearray_all[i - 1][READ]);
                close(pipearray_all[i - 1][WRITE]);
                close(pipearray_all[i][READ]);
                close(pipearray_all[i][WRITE]);

                execl("/bin/sh", "sh", "-c", commands[i], (char *)0);
                _exit(127); /* execl error */
            }
            else if (pid_all[i] > 0)
            { /* parent process */
                close(pipearray_all[i - 1][READ]);
                close(pipearray_all[i - 1][WRITE]);
                printf("Shsh pipe processing %s\n", commands[i]);
                printf("Shsh forked %d for shcmd, with in-pipe %d and out-pipe %d\n",
                       pid_all[i], pipearray_all[i - 1][READ], pipearray_all[i][WRITE]);
            }
        }

        if ((pid_all[num_command - 1] = fork()) < 0)
        {
            perror("fork error\n");
            exit(EXIT_FAILURE);
        }
        else if (pid_all[num_command - 1] == 0) /* the last child process */
        {
            if (pipeCToP[userid][WRITE] != STDOUT_FILENO)
                dup2(pipeCToP[userid][WRITE], STDOUT_FILENO);
            close(pipeCToP[userid][WRITE]);

            if (dup2(pipearray_all[num_command - 2][READ], STDIN_FILENO) == -1)
                perror("dup2\n");

            close(pipearray_all[num_command - 2][READ]);
            close(pipearray_all[num_command - 2][WRITE]);
            execl("/bin/sh", "sh", "-c", commands[num_command - 1], (char *)0);
            _exit(127); /* execl error */
        }
        else if (pid_all[num_command - 1] > 0)
        {
            /* parent process */
            close(pipearray_all[num_command - 2][READ]);
            close(pipearray_all[num_command - 2][WRITE]);
            printf("Shsh pipe processing %s\n", commands[num_command - 1]);
            printf("Shsh forked %d for shcmd, with in-pipe %d and out-pipe %d\n",
                   pid_all[num_command - 1], pipearray_all[num_command - 2][READ], STDOUT_FILENO);
        }
    }

    /* Wait for all children processes terminate */
    for (int i = 0; i < num_command; i++)
        waitpid(pid_all[i], NULL, 0);
}
