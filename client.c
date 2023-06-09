#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include "QuizDB.h"

#define BUFSIZE 1024

int main(int argc, char *argv[]) 
{
    if (argc != 3)
    {
       fprintf(stderr, "Usage: %s <IP address of server> <port number>.\n",
               argv[0]);
       exit(-1);
    }

    struct sockaddr_in serverAddress;
   
    memset(&serverAddress, 0, sizeof(struct sockaddr_in));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(argv[1]);
    serverAddress.sin_port = htons(atoi(argv[2]));

    int cfd = socket(AF_INET, SOCK_STREAM, 0);
    if (cfd == -1) {
       fprintf(stderr, "socket() error.\n");
       exit(-1);
    }

    int rc = connect(cfd, (struct sockaddr *)&serverAddress, 
                     sizeof(struct sockaddr));
    if (rc == -1) {
       fprintf(stderr, "connect() error, errno %d.\n", errno);
       exit(-1);
    }

    char buf[BUFSIZE];
    char questions[BUFSIZE];
    char* score = malloc(BUFSIZE);
    int nbytes, nbytes_questions;
    int answers = 0;
    char *quit = "q";
    int quit_flag = 0;

    while (1) {

        nbytes = recv(cfd, buf, BUFSIZE, 0); // receiving the prompt
        if (nbytes == -1) {
            fprintf(stderr, "recv() error.\n");
            exit(-1);
        } else if (nbytes == 0) { // the other connection has closed the socket (ctrl-C)
            break;
        }
        buf[nbytes] = '\0';
        fprintf(stdout, "%s", buf);
        fflush(stdout);
        char* input = malloc(BUFSIZE);
        size_t len = 0;
        ssize_t nread = getline(&input, &len, stdin);
            if (nread == -1) {
                fprintf(stderr, "getline() error.\n");
                exit(-1);
            }
        input[nread-1] = '\0';
        
        while((strcmp(input, quit) != 0) && (strcmp(input, "Y") != 0)){ // error check 
            fprintf(stdout, "Please only input 'q' or 'Y'\n");
            nread = getline(&input, &len, stdin);
            input[nread-1] = '\0';
        }
        send(cfd, input, strlen(input),0);
        if(strcmp(input, quit) == 0){
            exit(1);
        } 
        while(answers < 5 ) { // beginning of the quiz
            nbytes_questions = recv(cfd, questions, BUFSIZE, 0);
            if (nbytes_questions == -1) {
                fprintf(stderr, "recv() error.\n");
                exit(-1);
            } else if (nbytes_questions == 0) { // the other connection has closed the socket (ctrl-C)
                break;
            }
 
            fprintf(stdout, "%s", questions);
            fflush(stdout);
            fflush(stdin);
            char* client_answers = NULL;
            size_t len = 0;
            ssize_t nread = getline(&client_answers, &len, stdin);
            if (nread == -1) {
                fprintf(stderr, "getline() error.\n");
                exit(-1);
            }

            client_answers[nread-1] = '\0';
            send(cfd, client_answers, strlen(client_answers) ,0);
            answers++;
        }

        int nbytes_score = recv(cfd, score, BUFSIZE, 0);
        if (nbytes_score == -1) {
            fprintf(stderr, "recv() error.\n");
            exit(-1);
        } else if (nbytes_score == 0) { // the other connection has closed the socket (ctrl-C)
            break;
        }
        
        fprintf(stdout, "%s", score);
        free(score);
        quit_flag = 1;
        char send_quit_flag[16];
        sprintf(send_quit_flag, "%d", quit_flag);
        send(cfd, send_quit_flag, strlen(send_quit_flag), 0);
            exit(0);
    }

    close(cfd);
    return 0;

}