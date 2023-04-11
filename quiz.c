#include <stdlib.h>
#include <stdio.h>
#include "QuizDB.h"

int main(int argc, char** argv)
{
    int q, numq = sizeof(QuizQ)/sizeof(QuizQ[0]);
    for (q = 0; q < numq; q++)
    {
        printf("Q. %s\n", QuizQ[q]); 
        printf("A. %s\n", QuizA[q]); 
    }

    exit(EXIT_SUCCESS);
}

