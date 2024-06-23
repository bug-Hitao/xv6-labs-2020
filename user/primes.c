#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define RD 0
#define WR 1

void prime(int rd){
    int n;
    read(rd, &n, 4);
    printf("prime %d\n", n);
    int created = 0;
    int p[2];
    int num;
    while(read(rd, &num, 4) != 0){
        if(created == 0){
            pipe(p);
            created = 1;
            int pid = fork();
            if(pid == 0){
                close(p[WR]);
                prime(p[RD]);
                return;
            }else{
                close(p[RD]);
            }
        }
        if(num % n != 0){
            write(p[WR], &num, 4);
        }
    }
    close(rd);
    close(p[WR]);
    wait(0);
}

int
main(int argc, char *argv[])
{
    int p[2];
    pipe(p);

    if(fork() == 0){
        close(p[WR]);
        prime(p[RD]);
        close(p[RD]);
    }else{
        close(p[RD]);
        for (int i = 2; i <= 35 ; i++)
        {
            write(p[WR], &i, sizeof(i));
        }
        close(p[WR]);
        wait(0);
    }

    exit(0);
}