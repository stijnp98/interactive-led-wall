/*
 * File:   time_delay.c
 * Author: http://pic18fxx.blogspot.com
 */
#include <xc.h>
#include "time_delay.h"
#include "mcc_generated_files/tmr1.h"

void Delay_ms(unsigned int count){
unsigned int i;
for(i = 0; i < count; i++){
    TMR1 = 0;
    TMR1_Start();
    while(TMR1 < 4000);
}
}

void Delay_us(unsigned int count){
unsigned int i;
for(i = 0; i < count; i++){
    TMR1 = 0;
    while(TMR1 < 4);
}
}