/* Microchip Technology Inc. and its subsidiaries.  You may use this software 
 * and any derivatives exclusively with Microchip products. 
 * 
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS".  NO WARRANTIES, WHETHER 
 * EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED 
 * WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A 
 * PARTICULAR PURPOSE, OR ITS INTERACTION WITH MICROCHIP PRODUCTS, COMBINATION 
 * WITH ANY OTHER PRODUCTS, OR USE IN ANY APPLICATION. 
 *
 * IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
 * INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND 
 * WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS 
 * BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE 
 * FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS 
 * IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF 
 * ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * MICROCHIP PROVIDES THIS SOFTWARE CONDITIONALLY UPON YOUR ACCEPTANCE OF THESE 
 * TERMS. 
 */

/* 
 * File:   
 * Author: 
 * Comments:
 * Revision history: 
 */

// This is a guard condition so that contents of this file are not included
// more than once.  
#ifndef COMMANDSET_H
#define	COMMANDSET_H

#include <xc.h> // include processor files - each processor file is guarded.  

//STATUSCODES
#define REGISTER 10         //module is powered up so register to list
#define SHUTDOWN 11     //module empty so will go offline
#define OK       12     //nothing to report module is still there
#define STATUSREQ 13    //request the status of the module
#define REGISTEROK 14
#define GETTIMEOUTINT 15
#define SETTIMEOUTINT 16
#define TIMEOUTINT    17

//BATTERYCODES
#define BATPER 20       //returns the percentage (12bit)
#define BATLOW 21       //the battery is running low please replace
#define BATFAULT 22     //no battery connected 
#define BAT      23      //request battery percentage

//SENSESPECIFIC
#define MODVAL  40
#define GETLOWLIMIT 41
#define GETHIGHLIMIT 42
#define GETAVG  43
#define SETLOWLIMIT 44
#define SETHIGHLIMIT 45
#define SETAVG 46
#define GETMESINTERVAL 47
#define SETMESINTERVAL 48

//TESTS
#define SETBLUELED    50
#define SETGREENLED   51
#define SETREDLED     52
#define RESETBLUELED  53
#define RESETGREENLED  54
#define RESETREDLED  55

//SLEEP
#define GOTOSLEEP  60
#define WAKEUP     61
#define REQUESTSLEEP 62

uint16_t checkcommand(unsigned char* recvbuffer,unsigned char* sendbuffer);

#endif