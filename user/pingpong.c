#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[]){
    if(argc != 1){
        fprintf(2,"Usage: pingpong\n");
        exit(1);
    }

    int parent2child[2];
    int child2parent[2];

    pipe(parent2child);
    if (pipe(parent2child) < 0){
        fprintf(2,"pipe error\n");
        exit(1);
    }

    pipe(child2parent);
    if (pipe(child2parent) < 0){
        fprintf(2, "pipe error\n");
        exit(1);
    }

    char byte = 'a';
    int pid = fork();

    //[0]: read
    //[1]: write

    if (pid < 0){
        close (parent2child[0]);
        close (parent2child[1]);
        close (child2parent[0]);
        close (child2parent[1]);
        fprintf(2, "fork error\n");
        exit(1);
    }

    if (pid == 0){
        close(parent2child[1]);
        close(child2parent[0]);

        //parent - child - child read
        if(read(parent2child[0], &byte, 1) != 1){
            fprintf(2, "child read error\n");
            close(parent2child[0]);
            close(child2parent[1]);
            exit(1);
        }

        printf("%d: received ping\n", getpid());
        close(parent2child[0]);

        //child - parnet - child write
        if(write(child2parent[1], &byte, 1) != 1){
            fprintf(2, "child write error\n");
            close(child2parent[1]);
            exit(1);
        }

        close(child2parent[1]);
        //wait(0);
        exit(0);
    }

    close(parent2child[0]);
    close(child2parent[1]);

    //parent - child - parent write
    if(write(parent2child[1], &byte , 1) != 1){
        fprintf(2, "parent write error\n");
        close(parent2child[1]);
        close(child2parent[0]);
        exit(1);
    }

    close(parent2child[1]);

    //child - parent - parent read
    if(read(child2parent[0], &byte , 1) != 1){
        fprintf(2, "parent read error\n");
        close(child2parent[0]);
        exit(1);
    }

    printf("%d: receive pong\n", getpid());
    close(child2parent[0]);
    wait(0);
    exit(0);
}