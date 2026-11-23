#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>

#define SYS_GOTO_SLEEP 156
#define SYS_AWAKE 174

int main(void){
    int r;
    r = syscall(SYS_GOTO_SLEEP);
    printf("goto_sleep -> %d\n", r);
    r = syscall(SYS_AWAKE);
    printf("awake -> %d\n", r);

    return 0;
}