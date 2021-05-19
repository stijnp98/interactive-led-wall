#ifndef __CONNMGR_H__
#define __CONNMGR_H__

#include "lib/dplist.h"
#include "lib/tcpsock.h"
#include "config.h"

//function declarations
void connmgr_listen(int port_number);
void connmgr_free();

typedef struct{
	uint8_t id;
	uint8_t command;
    uint32_t data;
} sensor_data_t;

typedef struct{
	sensor_data_t sensor[10];
	uint8_t datainbuffer;
} sensor_data_buffer_t;

//structs declaration
typedef struct{
	tcpsock_t* socket;
	sensor_data_t reading;
}node_info;

typedef struct{
	tcpsock_t* socket;
	uint8_t nodeid;
	uint8_t command;
	uint32_t data;
	uint8_t sensortype;
	uint16_t battery;
	uint16_t upperlimit;
	uint16_t lowerlimit;
	uint32_t avgvalue;
	uint32_t lastenergy;
	uint8_t sleepmode;
	uint8_t messurementinterval;
}sensornode_info;


#endif
