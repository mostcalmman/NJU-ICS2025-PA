#include "sys/time.h"
#include <stdio.h>
#include <time.h>

int main(){
    struct timeval tv;
    gettimeofday(&tv, NULL);
    time_t t = tv.tv_sec * 1000000 + tv.tv_usec;
    int i = 0;
    while(i < 5){
        gettimeofday(&tv, NULL);
        if(tv.tv_sec * 1000000 + tv.tv_usec - t >= 500000){
            t = tv.tv_sec * 1000000 + tv.tv_usec;
            printf("Uptime: %ld seconds and %ld microseconds\n", tv.tv_sec, tv.tv_usec);
            i++;
        }
    }
}