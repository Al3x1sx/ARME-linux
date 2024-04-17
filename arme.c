#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

void main() {
    
    //initalisation des données
    int i;
    for (i=0; i<3; i++) {
        int pid_division;
        pid_division = fork();

        if(pid_division == 0) {
            printf("je suis le fils %d avec le pid %d\n", i, getpid());
        }
    }
    return;
}