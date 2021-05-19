/**
  Generated main.c file from MPLAB Code Configurator

  @Company
 KULeuven
 * engineer: Stijn Potters

  @File Name
    main.c

  @Summary
    This is the generated main.c using PIC24 / dsPIC33 / PIC32MM MCUs.

  @Description
    This source file provides main entry point for system initialization and application code development.
    Generation Information :
        Product Revision  :  PIC24 / dsPIC33 / PIC32MM MCUs - 1.170.0
        Device            :  PIC24FJ256GA705
    The generated drivers are tested against the following:
        Compiler          :  XC16 v1.61
        MPLAB 	          :  MPLAB X v5.45
*/

/*
    (c) 2020 Microchip Technology Inc. and its subsidiaries. You may use this
    software and any derivatives exclusively with Microchip products.

    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
    WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
    PARTICULAR PURPOSE, OR ITS INTERACTION WITH MICROCHIP PRODUCTS, COMBINATION
    WITH ANY OTHER PRODUCTS, OR USE IN ANY APPLICATION.

    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
    BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
    FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
    ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
    THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.

    MICROCHIP PROVIDES THIS SOFTWARE CONDITIONALLY UPON YOUR ACCEPTANCE OF THESE
    TERMS.
*/

/**
  Section: Included Files
*/
#include <xc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "config.h"
#include "nrf24l01.h"
#include "COMMANDSET.h"
#include "time_delay.h"
#include "mcc_generated_files/system.h"
#include "mcc_generated_files/pin_manager.h"
#include "mcc_generated_files/adc1.h"
#include "mcc_generated_files/tmr1.h"
#include "mcc_generated_files/tmr2.h"
#include "mcc_generated_files/spi2.h"

unsigned char ADDRESS_DATA_PIPE0[6] = "00026"; //gateway
unsigned char ADDRESS_DATA_PIPE1[6] = "00027"; //this node
#define SAMPLEBINS 2048
#define SAMPLEBINS_2 SAMPLEBINS/2
#define node 24

uint32_t timeout = 0;
uint8_t executetransfer = 0;
uint8_t checkforincommingdata = 0;
uint8_t entersleep = 0;
uint16_t count = 0;
uint8_t batteryok = 1;
uint8_t timerstate = 0;
uint16_t battery = 0;
uint8_t sendbatteryupdate = 1;
uint8_t poweruptheboard = 0;
uint8_t slcounter = 0;
uint8_t itemsinbuffer = 0;
unsigned char bufferQueue[3][6];
unsigned char bufferTX[6];
unsigned char bufferRX[6];

int main(void)
{
    // initialize the device
    SYSTEM_Initialize();
    powerUpBoard();
    TMR2_Period16BitSet(0x1C61);
    TMR2_Counter16BitSet(0x0000);
    TMR2_SetInterruptHandler(&handleTimerINT);
    TMR2_Start();
    timerstate = 2;
    startsweep();
    while (1)
    { 
        if(poweruptheboard)
        {
            powerUpBoard();
            poweruptheboard = 0;
        }
        if(sendbatteryupdate)
        {
            sendbatterystate();
            sendbatteryupdate = 0;
        }
        checkforincommingdata = 0;
        NRF24L01_SetMode(RX_MODE); 
            //the module will listen 300ms for useful data otherwise it will continue with it's regular operation
            Delay_ms(1000);
            while(NRF24L01_DataReady()){
                NRF24L01_ReadData(bufferRX);
                if(checkcommand(bufferRX,bufferTX))
                {                   
                    memcpy(*(bufferQueue + itemsinbuffer),bufferTX,6);
                    itemsinbuffer++;
                }
                IO_RC7_SetHigh();
                Delay_ms(10);
                IO_RC7_SetLow();
                checkforincommingdata = 1;
            }
            NRF24L01_WriteRegister(STATUSS, 0x70); // Clear STATUS.
            NRF24L01_Flush();
            
            NRF24L01_StandbyI();
            NRF24L01_SetMode(TX_MODE); 
            
            while(itemsinbuffer != 0)
            {
                itemsinbuffer--;
                NRF24L01_SendData(*(bufferQueue + itemsinbuffer));
                Delay_ms(30);
            }
            
        /*CHECK IF MORE DATA NEED TO BE FETCHED FROM THE GATEWAY*/
        if(!checkforincommingdata){
            NRF24L01_StandbyI();
            NRF24L01_CE = 0;
            if(timerstate == 1){
                entersleepmode();
            }
            else
            {
                Sleep();
            }
        }

    }

    return 1;
}
/**
 End of File
*/

uint16_t checkcommand(unsigned char* recvbuffer,unsigned char* sendbuffer){
    switch(recvbuffer[1])
    {
        case STATUSREQ:
            if(batteryok) sendbuffer[1] = OK; else sendbuffer[1] = BATLOW;
            return 1;
            break;
        case GETTIMEOUTINT:
            sendbuffer[0] = node;
            sendbuffer[1] = TIMEOUTINT;
            sendbuffer[2] = (unsigned char)((timeout >> 24) & 0x000000FF);
            sendbuffer[3] = (unsigned char)((timeout >> 16) & 0x000000FF);
            sendbuffer[4] = (unsigned char)((timeout >> 8) & 0x000000FF);
            sendbuffer[5] = (unsigned char)(timeout & 0x000000FF);
            return 1;
            break;
        case SETTIMEOUTINT:
            sendbuffer[0] = node;
            sendbuffer[1] = OK;
            timeout = (((uint32_t)recvbuffer[2]) << 24);
            timeout =+ (((uint32_t)recvbuffer[3]) << 16);
            timeout =+ (((uint32_t)recvbuffer[4]) << 8);
            timeout =+ (((uint32_t)recvbuffer[5]));
            return 1;
            break;
        case BAT:
            sendbuffer[0] = node;
            sendbuffer[1] = BATPER;
            battery = batterypercentage();
            sendbuffer[2] = 0;
            sendbuffer[3] = 0;
            sendbuffer[4] = (unsigned char)((battery >> 8) & 0xFF);
            sendbuffer[5] = (unsigned char)(battery & 0xFF);
            return 1;
            break;
        case SETBLUELED:
            IO_RC6_SetHigh();
            return 0;
            break;
        case RESETBLUELED:
            IO_RC6_SetLow();
            return 0;
            break;
        case SETGREENLED:
            IO_RC7_SetHigh();
            return 0;
            break;
        case RESETGREENLED:
            IO_RC7_SetLow();
            return 0;
            break;
        case SETREDLED:
            IO_RB9_SetHigh();
            return 0;
            break;
        case RESETREDLED:
            IO_RB9_SetLow();
            return 0;
            break;
        case GOTOSLEEP:
            sendbuffer[0] = node;
            sendbuffer[1] = OK;
            NRF24L01_StandbyI();
            NRF24L01_SetMode(TX_MODE); 
            Delay_ms(10);
            NRF24L01_SendData(bufferTX); 
            Delay_ms(10);
            NRF24L01_StandbyI();
            timerstate = 1;
            entersleep = 1;
            entersleepmode();
            return 0;
            break;
        case WAKEUP:
            startsweep();
            sendbuffer[0] = node;
            sendbuffer[1] = OK;
            entersleep = 0;
            timerstate = 2;
            return 1;
            break;         
    }
    return 1;
}

void handleTimerINT(){
    TMR2_Stop();
    TMR2_Period16BitSet(0x1C61);
    TMR2_Counter16BitSet(0x0000);
    TMR2_Start();
    if(timerstate == 1 && slcounter > 30){
        poweruptheboard = 1;
        checkforincommingdata = 1;
        slcounter = 0;
    }
    else if(slcounter > 30)
    {
        sendbatteryupdate = 1;
        checkforincommingdata = 1;
        slcounter = 0;
    }
    else
    {
        slcounter++;
        IO_RC6_SetHigh();
        IO_RB9_SetHigh();
        Delay_ms(20);
        IO_RC6_SetLow();
        IO_RB9_SetLow();
        Sleep();
    }
    
}

void entersleepmode(){
    void NRF24L01_PowerDown();
    TMR2_Period16BitSet(0x1C61);
    TMR2_Counter16BitSet(0x0000);
    powerDownBoard();
    TMR2_Start();
    timerstate = 1;
    Sleep(); // enter Idle mode
}

void powerUpBoard(){
    IO_RA0_SetHigh();
    IO_RC6_SetHigh();
    Delay_ms(100); //give some time to powerup
    IO_RC6_SetLow();
    unsigned char bufferTX[6] = {node,REGISTER,0,0,0,0};
    NRF24L01_Init(TX_MODE, 3,ADDRESS_DATA_PIPE0,ADDRESS_DATA_PIPE1); 
    NRF24L01_StandbyI();
    NRF24L01_SetMode(TX_MODE); 
    IO_RB9_SetHigh(); 
    NRF24L01_SendData(bufferTX); 
    Delay_ms(20);
    IO_RB9_SetLow(); 
    NRF24L01_StandbyI();
    NRF24L01_SetMode(RX_MODE);
    initsweep();
}

void powerDownBoard(){
    stopsweep();
    IO_RA1_SetHigh();
    IO_RA0_SetLow();
    IO_RC6_SetLow();
    IO_RC7_SetLow();
    IO_RB9_SetLow();
}

uint16_t batterypercentage(){
    IO_RB12_SetHigh();
    ADC1_Enable();
    Delay_ms(100);
    battery = ADC_Read12bit(channel_AN7);
    ADC1_Disable();
    IO_RB12_SetLow();
    return battery;
}

void sendbatterystate(){
    unsigned char sendbuffer[6] = {node,BATPER,0,0,0,0};
    uint16_t batteryvoltage = 0;
    NRF24L01_StandbyI();
    NRF24L01_SetMode(TX_MODE); 
    Delay_ms(10);
    IO_RC7_SetHigh(); 
    batteryvoltage = batterypercentage();
    if(battery < 2140){
        batteryok = 0;
    }
    IO_RC7_SetLow(); 
    sendbuffer[2] = 0;
    sendbuffer[3] = 0;
    sendbuffer[4] = (unsigned char)((batteryvoltage >> 8) & 0xFF);
    sendbuffer[5] = (unsigned char)(batteryvoltage & 0xFF);
    NRF24L01_SendData(sendbuffer); 
    Delay_ms(10);
    NRF24L01_StandbyI();
    checkforincommingdata = 1;
}

void initsweep(){
    IO_RB4_SetHigh(); //set TRISTATE
    IO_RB0_SetHigh(); //set CS
    IO_RA1_SetLow(); //set shutdown low
    IO_RB3_SetLow(); //set CONTROL LOW*/
    /*SEND CONFIGURATIONS TO THE SWEEP CHIP*/
    IO_RB0_SetLow(); //set CS_LOW
    SPI2_Exchange16bit(0b0000011010011111); //control register
    IO_RB0_SetHigh(); //set CS_LOW
    IO_RB0_SetLow(); //set CS_LOW
    SPI2_Exchange16bit(0b0001101111111111); //no of increments
    IO_RB0_SetHigh(); //set CS_LOW
    IO_RB0_SetLow(); //set CS_LOW
    SPI2_Exchange16bit(0b0010000000000001); //delta freq lower bits
    IO_RB0_SetHigh(); //set CS_LOW
    IO_RB0_SetLow(); //set CS_LOW
    SPI2_Exchange16bit(0b0011000000000000); //delta freq higher bits
    IO_RB0_SetHigh(); //set CS_LOW
    IO_RB0_SetLow(); //set CS_LOW
    SPI2_Exchange16bit(0b0110000101011010); //increment interval
    IO_RB0_SetHigh(); //set CS_LOW
    IO_RB0_SetLow(); //set CS_LOW
    SPI2_Exchange16bit(0b1000000000000000); //burst interval
    IO_RB0_SetHigh(); //set CS_LOW
    IO_RB0_SetLow(); //set CS_LOW
    SPI2_Exchange16bit(0b1100000100000000); //startfreq lower bits
    IO_RB0_SetHigh(); //set CS_LOW
    IO_RB0_SetLow(); //set CS_LOW
    SPI2_Exchange16bit(0b1101000000000010); //startfreq higher bits  
    IO_RB0_SetHigh(); //set CS_LOW
}

void startsweep(){
    IO_RB3_SetHigh(); //set CONTROL LOW
    Delay_ms(1);
    IO_RB3_SetLow(); //set CONTROL LOW
}

void stopsweep(){
    IO_RC0_SetHigh(); //set CONTROL LOW
    Delay_ms(1);
    IO_RC0_SetLow(); //set CONTROL LOW
}