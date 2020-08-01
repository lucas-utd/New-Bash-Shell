// Program:
//        This program is used for creating user mananger.
// History: 2020/02/23
// Author:  Tao Chen
// Version: 1.0
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>
#include <signal.h>

#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

#define READ 0
#define WRITE 1

#define MAXLINE 1024
#define COMMAND_LENGTH 10  /* the max length of each command */
#define MAX_NUM_COMMAND 10 /* the max commands in a pipe shell */
#define MAXFILENAME 128
#define MAXUSER 11 /* the max number useres of UM support, don't use the user zero */

int userStatus[MAXUSER] = {0}; //the status of users (when user i actives, then userStatus[i] = 1)
pthread_t tids[MAXUSER];       //the id array of threads which serve for users.
pid_t shshPid[MAXUSER];        //the process id for every shsh shell
int newSockfds[MAXUSER];       //the socket numbers which are used to communicate with users
pid_t userPid[MAXUSER];        //the process id for every user process

int sleepTime;    // the sleep time from terminal input
int portno;       // the port the server socket uses
int serverSockfd; // the server socket's number

void *shshThread(void *arg);

int pipePToC[MAXUSER][2]; // the pipes which use for send messages from UM to every user's shsh
int pipeCToP[MAXUSER][2]; // the pipes which use for send messages from every user's shsh to UM

void login_function(int userid);
void logout_function(int userid);

extern void shshcmd_function(int userid); // shsh shell main function

void error(char *msg) // the perror wrap function
{
    perror(msg);
    exit(0);
}

void catch_terminate(int sig_num) // signal handler
{
    int numUsers = 0, ret;
//    printf("You are killing this process.\n");
    for (int i = 1; i < MAXUSER; i++)
    {
        if (userStatus[i] != 0)
        {
            numUsers += userStatus[i];
            ret = write(newSockfds[i], "logout\n", strlen("logout\n"));
            if (ret < 0)
                error("ERROR writing to socket");

            logout_function(i);
        }
    }

    close(serverSockfd);
    printf("UM terminated on Admin request with %d active users.\n", numUsers);
    fflush(stdout);
    exit(0);
}

void catch_sleep(int sig_num) // signal handler
{
    printf("UM receive the sleep signal to sleep for %d seconds.\n", sleepTime);
    fflush(stdout);
    sleep(sleepTime);
}

void catch_infor(int sig_num) // signal handler
{
//    printf("A user signal is caught %d.\n", sig_num);
    int numUsers = 0;
    for (int i = 1; i < MAXUSER; i++)
        numUsers += userStatus[i];
    printf("UM's pid: %d, port number: %d, number of active users: %d.\n",
           getpid(), portno, numUsers);
    fflush(stdout);
}

void catch_listuser(int sig_num) // signal handler
{
//    printf("A user signal is caught %d.\n", sig_num);
    for (int i = 1; i < MAXUSER; i++)
        if (userStatus[i] != 0)
        {
            printf("Userid: %d, the pid of user's Shsh: %d, the socket number: %d.\n",
                   i, shshPid[i], newSockfds[i]);
        }
    fflush(stdout);
}

int main(int argc, char *argv[])
{
    // bind an interrupt bit to an interrupt handler function
    signal(SIGQUIT, catch_terminate);
    signal(SIGINT, catch_sleep);
    signal(SIGRTMAX, catch_infor);
    signal(SIGRTMIN, catch_listuser);

    int newsockfd, clilen;
    struct sockaddr_in serv_addr, cli_addr;
    int ret;

    pthread_t tmptid;

    if (argc < 3)
    {
        fprintf(stderr, "usage %s port sleeptime.\n", argv[0]);
        exit(0);
    }

    sleepTime = atoi(argv[2]);
    if (sleepTime == 0 || sleepTime > 10)
    {
        fprintf(stderr, "Sleep Time must between 0 and 10, not equal to 0 or 10.\n");
        exit(0);
    }

    serverSockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSockfd < 0)
        error("ERROR opening socket");
    bzero((char *)&serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(serverSockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR binding");

    listen(serverSockfd, 5);

    printf("UM server socket ready, and UM's pid is %d.\n", getpid());

    while (1)
    {
        clilen = sizeof(cli_addr);
        newsockfd = accept(serverSockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0)
            error("ERROR accepting");
        else
            printf("Accept client socket %d, %d\n",
                   newsockfd, (int)cli_addr.sin_port);
        // Create thread for every user and its parameter is the socket number
        pthread_create(&tmptid, NULL, shshThread, (void *)(intptr_t)newsockfd);
    }

    exit(0);
}

void *shshThread(void *arg)
{
    char buf[MAXLINE];
    char userid[COMMAND_LENGTH]; /* the input userid */
    char remain_buff[MAXLINE];   /* the remain of buff which deletes the userid */
    char backCommand[MAXLINE];   // save the previous command and use for pre command
    int requirePre = 0, ret, newSockfd;

    newSockfd = (intptr_t)arg;

    while (1)
    {
        bzero(buf, MAXLINE);
        ret = read(newSockfd, buf, MAXLINE - 1);
        if (ret < 0)
            error("ERROR reading command from socket");


        if (buf[strlen(buf) - 1] == '\n')
            buf[strlen(buf) - 1] = '\0'; /* replace newline with null */

        /* getting the userid */
        int i = 0;
        int j = 0;

        while (buf[j] != '\0' && (userid[i] = buf[j]) != ' ')
        {
            i++;
            j++;
        }
        userid[i] = '\0'; /* replace the end char with '\0' */

        while (buf[j] != '\0' && buf[j] == ' ') /* skipping the space of the front remaining buf */
            j++;

        /* getting the remain of buff */
        for (int m = 0; m < MAXLINE; m++)
            remain_buff[m] = '\0';
        i = 0;
        while (buf[j] >= 0 && buf[j] <= 127 && buf[j] != '\0')
        {
            remain_buff[i++] = buf[j++];
        }
        remain_buff[i] = '\0'; /* replace the end char with '\0' */

        /* delete the end space of remain_buff */
        i--;
        while (remain_buff[i] == ' ')
        {
            remain_buff[i] = '\0';
            i--;
        }

        int id = atoi(userid);
        if (strncmp(remain_buff, "login", 5) == 0)
        {
            userStatus[id] = 1;         //Save the status of user.
            newSockfds[id] = newSockfd; //Save the socket number which serve for user.
            tids[id] = pthread_self();

            printf("New user %d logs in, thread created, socket %d with port %d is used.\n",
                   id, newSockfd, portno);
            fflush(stdout);

            login_function(id);
        }
        else if (strncmp(remain_buff, "logout", 6) == 0 || strncmp(remain_buff, "exit", 4) == 0)
        {
            logout_function(id);
            return NULL; // exit the thread
        }
        else if (strncmp(remain_buff, "pre", 3) == 0 || strncmp(remain_buff, "cmd", 3) == 0 ||
                 strncmp(remain_buff, "pipe", 4) == 0)
        {
            if (strncmp(remain_buff, "pre", 3) == 0 && requirePre == 1)
            {
                char *pointInput;
                char backInput[MAXLINE];
                pointInput = strchr(remain_buff, ' ');
                pointInput++;
                strcpy(backInput, pointInput);

                /* Splice the previous command with input that pre provides */
                sprintf(remain_buff, "cmd %s %s\n", backCommand, backInput);
                requirePre = 0;
            }
            else
            {
                sprintf(remain_buff, "%s\n", remain_buff);
            }

            /* Send the command to pipe where shsh receives it. */
            write(pipePToC[id][WRITE], remain_buff, strlen(remain_buff) + 1);

            /* Wait the shsh finishs command execution. */
            sleep(0.3);

            char shshout[MAXLINE]; // shsh output
            for (i = 0; i < MAXLINE; i++)
                shshout[i] = '\0';
            int stopreadshshout = 0;
            /* Receive the output of shsh */
            while (stopreadshshout == 0 && read(pipeCToP[id][READ], shshout, MAXLINE) > 0)
            {
                if (shshout[strlen(buf) - 1] == '\n')
                    shshout[strlen(buf) - 1] = '\0'; /* replace newline with null */

                char *shshoutpoint = shshout;
                char delim[] = "\n";
                char *token;
                /* split the output of shsh using "\n" */
                for (token = strsep(&shshoutpoint, delim); token != NULL; token = strsep(&shshoutpoint, delim))
                { /* Previous command finished execution */
                    if (strncmp(token, "cmd>", 4) == 0)
                    {
                        stopreadshshout = 1;
                        break;
                    } /* Previous command need pre to get input */
                    else if (strncmp(token, "NEEDPRE", 7) == 0)
                    {
                        char *pointCommand;
                        pointCommand = strchr(token, ' ');
                        pointCommand++;
                        strcpy(backCommand, pointCommand);

                        char backInformation[MAXLINE];
                        sprintf(backInformation, "Command %s need pre to get input.", backCommand);

                        ret = write(newSockfd, backInformation, strlen(backInformation));
                        if (ret < 0)
                            error("ERROR writing to socket");

                        requirePre = 1;
                    }
                    else if (strncmp(token, "\n", 1) != 0)
                    {
                        /* write back the output of last command to socket */
                        ret = write(newSockfd, token, strlen(token));
                        if (ret < 0)
                            error("ERROR writing to socket");
                    }
                }
            }
        }
    }
}

void login_function(int userid)
{
    // create userid.dir, change to this directory.
    char directoryname[MAXFILENAME];
    sprintf(directoryname, "%d.dir", userid);
    mkdir(directoryname, 0700);

    pipe(pipePToC[userid]);
    pipe(pipeCToP[userid]);

    if ((shshPid[userid] = fork()) < 0)
        perror("fork() error\n");
    else if (shshPid[userid] == 0)
    {
        close(pipePToC[userid][WRITE]);
        close(pipeCToP[userid][READ]);

        // restore the signal handler for child process.
        signal(SIGQUIT, SIG_DFL);
        signal(SIGINT, SIG_DFL);
        signal(SIGRTMAX, SIG_DFL);
        signal(SIGRTMIN, SIG_DFL);

        /* enter the shsh shell */
        shshcmd_function(userid);
    }
    else if (shshPid[userid] > 0)
    {
        /* UM sends the home directory to the shsh */
        char homeFolder[MAXLINE];
        sprintf(homeFolder, "%d  %d.dir\n", userid, userid);
        write(pipePToC[userid][WRITE], homeFolder, strlen(homeFolder) + 1);

        printf("Shsh process (userid = %d, pid = %d) has been created by process %d\n",
               userid, shshPid[userid], getppid());
        fflush(stdout);
    }
}

void logout_function(int userid)
{
    write(pipePToC[userid][WRITE], "exit\n", sizeof("exit\n"));

    /* Wait the shsh exit process */
    sleep(1);

    userStatus[userid] = 0;      
    close(pipePToC[userid][WRITE]);
    close(pipeCToP[userid][READ]);
    printf("User %d logs out, socket %d has been closed, thread is terminating.\n",
           userid, newSockfds[userid]);
    fflush(stdout);
    pthread_cancel(tids[userid]);  
    close(newSockfds[userid]);
}
