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

unsigned char ADDRESS_DATA_PIPE0[6] = "00010"; //gateway
unsigned char ADDRESS_DATA_PIPE1[6] = "00017"; //this node
#define SAMPLEBINS 2048
#define SAMPLEBINS_2 SAMPLEBINS/2
#define node 14

uint16_t samples[SAMPLEBINS];
uint32_t avgenergy;
uint32_t biasvalue;
uint32_t nominalenergyrail = 340000;
uint32_t timeout = 0;
uint32_t upperlimit = 60000;
uint32_t lowerlimit = 60000;
uint8_t prevstate = 1;
uint8_t executetransfer = 0;
uint8_t checkforincommingdata = 0;
uint8_t entersleep = 0;
uint16_t noofsamples = 0;
uint16_t count = 0;
uint8_t batteryok = 1;
uint8_t timerstate = 0;
uint16_t battery = 0;
uint16_t measurementinterval = 400;
uint8_t sendbatteryupdate = 0;
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
    while (1)
    {
        /*VARIABLE DECLARATION*/
        uint16_t noofsamples = 0;
        
        /*FIRST CHECK IF THE BOARD IS POWERED UP*/
        if(poweruptheboard)
        {
            powerUpBoard();
            poweruptheboard = 0;
            checkforincommingdata = 1;
        }
        
        /*FIRST CHECK FOR INCOMMING DATA*/
        if(checkforincommingdata){
            NRF24L01_SetMode(RX_MODE); 
            //the module will listen 300ms for useful data otherwise it will continue with it's regular operation
            Delay_ms(500);
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
            NRF24L01_StandbyI();
            NRF24L01_CE = 0;
            checkforincommingdata = 0;
        }
        else
        {
            NRF24L01_StandbyI();
            NRF24L01_CE = 0;
        }
        
        Delay_ms(300);
        /*READ THE VALUES FROM THE PIEZO ELEMENT*/
        ADC1_Enable();
        for(noofsamples = 0;noofsamples < 20;noofsamples++); //wait some time
        for(noofsamples = 0;noofsamples < SAMPLEBINS;noofsamples++)
        {
            TMR1 = 0;
            samples[noofsamples] = ADC_Read12bit(channel_AN9);
            while(TMR1 < 40); //10µS
        }
        ADC1_Disable();
        
        /*CALCULATE THE AVG BIAS LEVEL BECAUSE THIS CAN DRIFT WITH TEMPERATURE AND BATTERYVOLTAGE!*/
        avgenergy = 0;
        biasvalue = 0;
        for(noofsamples = 0;noofsamples < SAMPLEBINS;noofsamples++)
        {
                biasvalue += samples[noofsamples];
        }
        biasvalue = biasvalue/2048;
        
        /*CALCULATE THE AVERAGE ENERGY ON THE HANDRAILMODULE*/
        for(noofsamples = 0;noofsamples < SAMPLEBINS;noofsamples++)
        {
                avgenergy += (abs(((int16_t)samples[noofsamples]) - ((int16_t)biasvalue)));
        }
        
        /*CHECK IF THE AVGENERGY OF THE HANDRAIL IS HIGHER OR LOWER THAN THE SETTED LIMITS*/
        if((avgenergy < (nominalenergyrail - lowerlimit)) || (avgenergy > (nominalenergyrail + upperlimit)))
        {
                IO_RC8_SetHigh();
                Delay_ms(10);
                IO_RC8_SetLow();
                prevstate = 1;
                executetransfer = 1;
        }
        else
        {
            if(prevstate)
            {
                IO_RC8_SetHigh();
                Delay_ms(10);
                IO_RC8_SetLow();
                executetransfer = 1;
            }
            else
            {
                executetransfer = 0;
                NRF24L01_StandbyI();
            }
            prevstate = 0;         
        }
        
        /*CHECK OR THE MODULE NEEDS TO TRANSMIT DATA TO THE GATEWAY*/
        if(executetransfer)
        {
            NRF24L01_StandbyI();
            NRF24L01_SetMode(TX_MODE); 
            Delay_ms(10);
            bufferTX[0] = node;
            bufferTX[1] = MODVAL;
            bufferTX[2] = (unsigned char)((avgenergy >> 24) & 0x000000FF);
            bufferTX[3] = (unsigned char)((avgenergy >> 16) & 0x000000FF);
            bufferTX[4] = (unsigned char)((avgenergy >> 8) & 0x000000FF);
            bufferTX[5] = (unsigned char)(avgenergy & 0x000000FF);
            NRF24L01_SendData(bufferTX); 
            Delay_ms(5);
            checkforincommingdata = 1;
        }
        if(sendbatteryupdate)
        {
            sendbatterystate();
            sendbatteryupdate = 0;
            checkforincommingdata = 1;
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
            timeout = (uint32_t)recvbuffer[5] | ((uint32_t)recvbuffer[4] << 8) | ((uint32_t)recvbuffer[3] << 16) | ((uint32_t)recvbuffer[2] << 24);
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
        case GETLOWLIMIT:
            sendbuffer[0] = node;
            sendbuffer[1] = GETLOWLIMIT;
            sendbuffer[2] = 0;
            sendbuffer[3] = 0;
            sendbuffer[4] = (unsigned char)((lowerlimit >> 8) & 0xFF);
            sendbuffer[5] = (unsigned char)(lowerlimit & 0xFF);
            return 1;
            break;
        case GETHIGHLIMIT:
            sendbuffer[0] = node;
            sendbuffer[1] = GETHIGHLIMIT;
            sendbuffer[2] = 0;
            sendbuffer[3] = 0;
            sendbuffer[4] = (unsigned char)((upperlimit >> 8) & 0xFF);
            sendbuffer[5] = (unsigned char)(upperlimit & 0xFF);
            return 1;
            break;
        case GETAVG:
            sendbuffer[0] = node;
            sendbuffer[1] = GETAVG;
            sendbuffer[2] = (unsigned char)((nominalenergyrail >> 24) & 0x000000FF);
            sendbuffer[3] = (unsigned char)((nominalenergyrail >> 16) & 0x000000FF);
            sendbuffer[4] = (unsigned char)((nominalenergyrail >> 8) & 0x000000FF);
            sendbuffer[5] = (unsigned char)(nominalenergyrail & 0x000000FF);
            return 1;
            break;
        case SETAVG:
            sendbuffer[0] = node;
            sendbuffer[1] = OK;
            nominalenergyrail = (uint32_t)recvbuffer[5] | ((uint32_t)recvbuffer[4] << 8) | ((uint32_t)recvbuffer[3] << 16) | ((uint32_t)recvbuffer[2] << 24);
            return 1;
            break;
        case SETLOWLIMIT:
            sendbuffer[0] = node;
            sendbuffer[1] = OK;
            lowerlimit = (uint32_t)recvbuffer[5] | ((uint32_t)recvbuffer[4] << 8) | ((uint32_t)recvbuffer[3] << 16) | ((uint32_t)recvbuffer[2] << 24);
            return 1;
            break;
        case SETHIGHLIMIT:
            sendbuffer[0] = node;
            sendbuffer[1] = OK;
            upperlimit = (uint32_t)recvbuffer[5] | ((uint32_t)recvbuffer[4] << 8) | ((uint32_t)recvbuffer[3] << 16) | ((uint32_t)recvbuffer[2] << 24);
            return 1;
            break;
        case GETMESINTERVAL:
            sendbuffer[0] = node;
            sendbuffer[1] = GETMESINTERVAL;
            sendbuffer[2] = 0;
            sendbuffer[3] = 0;
            sendbuffer[4] = (unsigned char)((measurementinterval >> 8) & 0xFF);
            sendbuffer[5] = (unsigned char)(measurementinterval & 0xFF);
            return 1;
            break;
        case SETMESINTERVAL:
            sendbuffer[0] = node;
            sendbuffer[1] = OK;
            measurementinterval = (uint32_t)recvbuffer[5] | ((uint32_t)recvbuffer[4] << 8);
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
            IO_RC8_SetHigh();
            return 0;
            break;
        case RESETREDLED:
            IO_RC8_SetLow();
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
            entersleep = 1;
            entersleepmode();
            return 0;
            break;
        case WAKEUP:
            sendbuffer[0] = node;
            sendbuffer[1] = OK;
            entersleep = 0;
            return 0;
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
        Delay_ms(20);
        IO_RC6_SetLow();    
        Sleep();
    }
}

void entersleepmode(){
    TMR2_Period16BitSet(0x1C61);
    TMR2_Counter16BitSet(0x0000);
    TMR2_SetInterruptHandler(handleTimerINT);
    powerDownBoard();
    TMR2_Start();
    timerstate = 1;
    Sleep(); // enter Idle mode
}

void powerUpBoard(){
    IO_RB14_SetHigh();
    IO_RC6_SetHigh();
    Delay_ms(20); //give some time to powerup
    IO_RC6_SetLow();
    unsigned char bufferTX[6] = {node,REGISTER,0,0,0,0};
    NRF24L01_Init(TX_MODE, 2,ADDRESS_DATA_PIPE0,ADDRESS_DATA_PIPE1); 
    NRF24L01_StandbyI();
    NRF24L01_SetMode(TX_MODE); 
    Delay_ms(10);
    NRF24L01_SendData(bufferTX); 
    NRF24L01_StandbyI();
    //NRF24L01_SetMode(RX_MODE);
    checkforincommingdata = 1;
}

void powerDownBoard(){
    IO_RB14_SetLow();
    IO_RC6_SetLow();
    IO_RC7_SetLow();
    IO_RC8_SetLow();
}

uint16_t batterypercentage(){
    IO_RB13_SetHigh();
    ADC1_Enable();
    Delay_ms(100);
    for(noofsamples = 0;noofsamples < 20;noofsamples++); //wait some time
    battery = ADC_Read12bit(channel_AN8);
    ADC1_Disable();
    IO_RB13_SetLow();
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
    IO_RC6_SetHigh(); 
    sendbuffer[2] = 0;
    sendbuffer[3] = 0;
    sendbuffer[4] = (unsigned char)((batteryvoltage >> 8) & 0xFF);
    sendbuffer[5] = (unsigned char)(batteryvoltage & 0xFF);
    NRF24L01_SendData(sendbuffer); 
    Delay_ms(10);
    IO_RC6_SetLow(); 
    NRF24L01_StandbyI();
    checkforincommingdata = 1;
}