#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#define MAXLINE 1024

int main(int argc, char *argv[])
{
    int pid;
    char buf[MAXLINE];
    char command[MAXLINE];     /* the input userid */
    char remain_buff[MAXLINE]; /* the remain of buff which deletes the userid */
    char backCommand[MAXLINE];

    if (argc < 2)
    {
        fprintf(stderr, "usage %s UM's pid.\n", argv[0]);
        exit(0);
    }

    if ((pid = atoi(argv[1])) == 0)
    {
        fprintf(stderr, "Input pid: %s is not effective.\n", argv[1]);
        exit(0);
    }

    while (1)
    {
        printf("admin> ");
        fflush(stdout);

        if (fgets(buf, MAXLINE, stdin) == NULL)
            exit(0);

        if (buf[strlen(buf) - 1] == '\n')
            buf[strlen(buf) - 1] = '\0'; /* replace newline with null */

        /* getting the userid */
        int i = 0;
        int j = 0;
        while (buf[j] != '\0' && buf[j] == ' ') /* skipping the space */
            j++;

        while ((command[i] = buf[j]) != '\0')
        {
            i++;
            j++;
        }
        command[i] = '\0'; /* replace the end char with '\0' */

        /* delete the end space of command */
        i--;
        while (command[i] == ' ')
        {
            command[i] = '\0';
            i--;
        }

        if (strncmp(command, "terminate", 9) != 0 && strncmp(command, "sleep", 5) != 0 &&
            strncmp(command, "infor", 5) != 0 && strncmp(command, "listuser", 8) != 0)
        {
            printf("The Admin can only use commands [terminate|sleep|infor|listuser].\n");
            continue;
        }

        if (strncmp(command, "terminate", 9) == 0)
        {
            kill(pid, SIGQUIT);
            sleep(5);
            exit(0);
        }
        else if (strncmp(command, "sleep", 5) == 0)
        {
            kill(pid, SIGINT);
//            sleep(5);
        }
        else if (strncmp(command, "infor", 5) == 0)
        {
            kill(pid, SIGRTMAX);
            sleep(1);
        }
        else if (strncmp(command, "listuser", 8) == 0)
        {
            kill(pid, SIGRTMIN);
            sleep(1);
        }
    }
}
