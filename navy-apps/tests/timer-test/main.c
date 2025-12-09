#include "sys/time.h"
#include <stdio.h>
#include <time.h>
uint32_t NDL_GetTicks();

int main(){
    uint32_t t = NDL_GetTicks();
    int i = 0;
    while (i<5) {
        uint32_t t_now = NDL_GetTicks();
        if (t_now - t >= 500000) {
            printf("0.5 second passed! NDL_GetTicks = %u us\n", t_now);
            t = t_now;
            i++;
        }
    }
}