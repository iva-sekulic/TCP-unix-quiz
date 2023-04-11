#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>
#include "QuizDB.h"
#include <sys/wait.h>

#define BACKLOG 128
#define BUFSIZE 1024
//#define quit "q\n"

int main(int argc, char *argv[]) 
{
    char prompt[] = "Welcome to Unix Programming Quiz\nThe quiz comprises five questions posed to you one after the other.\nYou have only one attempt to answer a question.\nYour final score will be sent to you after conclusion of the quiz.\nTo start the quiz, press Y and <enter>.\nTo quit the quiz, press q and <enter>.\n";
    char Y_q[BUFSIZE];
    if (argc != 3)
    {
       fprintf(stderr, "Usage: %s <IPv4 address> <port number>\n",
               argv[0]);
       exit(-1);
    }

    char* ip_address = argv[1];
    int port_number = atoi(argv[2]);
    struct sockaddr_in serverAddress;
    
    memset(&serverAddress, 0, sizeof(struct sockaddr_in));

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(ip_address);
    serverAddress.sin_port = htons(port_number);

    int addrlen = sizeof(serverAddress);
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd == -1) {
       fprintf(stderr, "socket() error.\n");
       exit(-1);
    }

    int optval = 1;
    if (setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR,
                   &optval, sizeof(optval)) == -1)
       exit(-1);

    int rc = bind(lfd, (struct sockaddr *)&serverAddress, sizeof(struct sockaddr));
    if (rc == -1) {
       fprintf(stderr, "bind() error.\n");
       exit(-1);
    }

    if (listen(lfd, BACKLOG) == -1)
       exit(-1);

    fprintf(stdout, "<Listening on %s:%d>\n<Press ctrl-C to terminate>\n", ip_address, port_number);

    srand(time(NULL)); // seed the random number generator
    int total_questions = 43;
    char *quit = "q";
    int quit_flag = 0;
    int questions[5];
    for (int i = 0; i < 5; i++) {
        questions[i] = rand() % total_questions; // select a random number between 0 and 44
    }

    while(1)  {

    // establishing a connection
    int cfd = accept(lfd, (struct sockaddr*)&serverAddress, (socklen_t*)&addrlen);
    if (cfd == -1) {
        continue; 
    }    

        switch(fork()) {
            case -1: 
                perror("fork()");
                exit(-1);
                break;
            case 0: 
                close(lfd);
                if(-1 == getpeername(cfd, (struct sockaddr*)&serverAddress, (socklen_t*)&addrlen)){
                    perror("getpeername()");
                } else {
                    if (send(cfd, prompt, strlen(prompt), 0) == -1) {
                        perror("send()");
                        exit(-1);
                    }
                    //char incorrect_message[BUFSIZE];
                    //char * incorrect_message = "Wrong Answer. Right Answer is ";
                    int num_sent = 0;
                    int score = 0;
                    int y_q = recv(cfd, Y_q, 1, 0);
                    Y_q[y_q] = '\0';
                    if(strcmp(Y_q, quit) == 0){
                        quit_flag = 1;
                        exit(0);
                    } else { // we are going to start the quiz here 
                        while (num_sent < 5) {
                            int question_number = questions[num_sent];
                            char* question = QuizQ[question_number];
                            char* answer = QuizA[question_number];
                            int question_nbytes = send(cfd, question, strlen(question), 0);
                            if (question_nbytes == -1) {
                                fprintf(stderr, "send() error.\n");
                                break;
                            }

                            // receive the client's answer
                            char buf[32];
                            question_nbytes = recv(cfd, buf, 32, 0);
                            if (question_nbytes == -1) {
                                fprintf(stderr, "recv() error.\n");
                                break;
                            } else if (question_nbytes == 0) {
                                fprintf(stderr, "Client closed connection.\n");
                                break;
                            }
                            buf[question_nbytes] = '\0';

                            // check if the answer is correct and send the result to the client
                            if (strcmp(buf, answer) == 0) {
                                question_nbytes = send(cfd, "Correct\n", 8, 0);
                                score++;
                            } else {
                                question_nbytes = send(cfd, "Incorrect\n", 10, 0);
                                //memset(incorrect_message, 0, sizeof(incorrect_message));
                                //strcpy(incorrect_message, "Wrong Answer. Right Answer is ");
                                //strcat(incorrect_message, answer);
                                //question_nbytes = send(cfd, incorrect_message, strlen(incorrect_message), 0);
                                //question_nbytes = send(cfd, answer, strlen(answer), 0);
                                //question_nbytes = send(cfd, "\n", 2, 0);
                            }

                            if (question_nbytes == -1) {
                                fprintf(stderr, "send() error.\n");
                                break;
                            }

                            num_sent++;
                        }

                        char score_message[50];
                        sprintf(score_message, "Your quiz score is %d/5. Goodbye!\n", score);
                        send(cfd, score_message, sizeof(score_message)+1, 0);
                    }

                }
                if (quit_flag == 1)
                    exit(0);
                break;
            default:
                wait(NULL);
                exit(0);
                break;
        }
    }
        close(lfd);
        return 0;
}

