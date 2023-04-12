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
int array_contains(int* array, int size, int value);

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

    while(1)  {
        // generating random questions
        for (int i = 0; i < 5; i++) {
            int rand_num;
            do {
                rand_num = rand() % total_questions;
            } while (array_contains(questions, i, rand_num)); // check if the array already contains the random number
            questions[i] = rand_num;
        }
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
                        if (send(cfd, prompt, strlen(prompt), 0) == -1) { // sending prompt
                            perror("send()");
                            exit(-1);
                        }

                        char * incorrect_message = "Wrong Answer. Right Answer is ";
                        int num_sent = 0;
                        int score = 0;
                        int y_q = recv(cfd, Y_q, 1, 0);
                        Y_q[y_q] = '\0';
                        if(strcmp(Y_q, quit) == 0){
                            quit_flag = 1;
                            exit(0);
                        } else { // starting the quiz here

                            char full_message[BUFSIZE] = "";
                            int question_nbytes;
                            while (num_sent < 5) {
                                int question_number = questions[num_sent];
                                char* question = QuizQ[question_number];
                                char* answer = QuizA[question_number];

                                strcat(full_message, question);
                                strcat(full_message, "\n");
                                question_nbytes = send(cfd, full_message, strlen(full_message)+1, 0); // +1 for NULL

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

                                    full_message[0] = '\0';
                                    strcat(full_message, "Correct\n");

                                    score++;
                                } else {

                                    full_message[0] = '\0';
                                    strcat(full_message, incorrect_message);
                                    strcat(full_message, answer);
                                    strcat(full_message, ".\n");

                                }

                                num_sent++;
                            }

                            char score_message[50];
                            sprintf(score_message, "Your quiz score is %d/5. Goodbye!\n", score);

                            strcat(full_message, score_message);
                            question_nbytes = send(cfd, full_message, strlen(full_message)+1, 0);
                            if (question_nbytes == -1) {
                                fprintf(stderr, "send() error.\n");
                                exit(0);
                            }
                            // the receiver telling the server that it finished the quiz
                            char receive_quit[16];
                            int quit_nbytes = recv(cfd,receive_quit, 16, 0);
                            if (quit_nbytes == -1) {
                                fprintf(stderr, "recv() error.\n");
                                break;
                            } else if (quit_nbytes == 0) {
                                fprintf(stderr, "Client closed connection.\n");
                                break;
                            }
                            int quit_received = 0;
                            quit_received = atoi(receive_quit);
                            if(quit_received == 1) {
                                exit(0);
                            }
                        }

                    }

                    break;
                default:
                    wait(NULL);
                    break;
            }
    }
            close(lfd);
            return 0;
}

// function to check if an array contains a value
int array_contains(int* array, int size, int value) {
    for (int i = 0; i < size; i++) {
        if (array[i] == value) {
            return 1;
        }
    }
    return 0;
}

