#ifndef _COMMAND_H_
#define _COMMAND_H_

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

#endif