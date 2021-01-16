#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include <pthread.h>
#include <stdint.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

#define MAXLINE 1024
#define COMMAND_LENGTH 10 /* the max length of each command */
#define MAXUSER 11        /* the max number useres of UM support, don't use the user zero */

int userid;

void error(char *msg)
{
    perror(msg);
    exit(0);
}


int main(int argc, char *argv[])
{
    int sockfd, portno, ret;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char *um_host_name;
    char command[MAXLINE]; /* the input command */
    char buffer[256];

    if (argc < 4)
    {
        fprintf(stderr, "usage %s userid host_name port\n", argv[0]);
        exit(0);
    }

    userid = atoi(argv[1]);
    if (userid >= MAXUSER || userid == 0)
    {
        fprintf(stderr, "The userid is between 1 to 10.\n");
        exit(0);
    }

    um_host_name = argv[2];

    portno = atoi(argv[3]);
    if ((portno = atoi(argv[3])) < 1203)
    {
        fprintf(stderr, "The port number is above 1204.\n");
        exit(0);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    server = gethostbyname(um_host_name);
    if (server == NULL)
        error("ERROR, no such host");
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr_list[0],
          (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR connecting");

    printf("User %d socket %d with port number %d has been created.\n",
           userid, sockfd, portno);
    fflush(stdout);

    sprintf(command, "%d login\n", userid);
    ret = write(sockfd, command, strlen(command));
    if (ret < 0)
        error("ERROR writing to socket");

    char buf[MAXLINE];
    int numRead;
    int printPrompt = 1;

    // make read from stdin and socket as NON-BLOCK status
    fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK);
    fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL) | O_NONBLOCK);

    while (1)
    {
        if (printPrompt == 1)
        {
            printf("ucmd> ");
            fflush(NULL);
            printPrompt = 0;
        }

        numRead = read(STDIN_FILENO, buf, MAXLINE);
        if (numRead > 0)
        {
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

            if (strncmp(command, "cmd", 3) != 0 && strncmp(command, "pipe", 4) != 0 && strncmp(command, "logout", 5) != 0 && strncmp(command, "exit", 4) != 0)
            {
                printf("Users can only use commands [cmd|pipe|logout|exit].\n");
                continue;
            }

            // add the user id for every command.
            char newCommand[MAXLINE];
            sprintf(newCommand, "%d %s\n", userid, command);
            ret = write(sockfd, newCommand, strlen(newCommand));
            if (ret < 0)
                error("ERROR writing to socket");

            if (strncmp(command, "exit", 4) == 0 || strncmp(command, "logout", 6) == 0)
            {
                sleep(2);
                exit(0);
            }

            printPrompt = 1;
        }

        sleep(1);

        bzero(buffer, MAXLINE);
        ret = read(sockfd, buffer, MAXLINE - 1);
        if (ret > 0)
        {
            if (strncmp(buffer, "   ", 3) != 0 && strncmp(buffer, "logout", 6) != 0)
            {
                printf("%s\n", buffer);
                fflush(stdout);
            }
            else if (strncmp(buffer, "logout", 6) == 0)
            { // receive the stop instruction from UM
                printf("\n");
                printf("User %d socket %d has been closed, process terminating.\n", userid, sockfd);
                fflush(stdout);
                exit(0);
            }
        }
    }
}
