#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    
    if (argc != 2) { 
    fprintf(2, "usage: sleep <time>\n");
    exit(1);
  }
    int num = atoi(argv[1]);    
    if(argc == 2){
    sleep(num);
    exit(0);
  }

    exit(0);
}