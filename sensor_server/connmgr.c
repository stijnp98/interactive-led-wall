#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <assert.h>
#include "config.h"
#include "lib/tcpsock.h"
#include <poll.h>
#include "connmgr.h"
#include "command.h"
#include "config.h"
#include <mongoc/mongoc.h>
#include <bson/bson.h>

#define MAX_CONN 10 // state the max. number of connections the server will handle before exiting

/* Implements a sequential test server (only one connection at the same time)
 */

void node_free(void ** element);
int node_compare(void * x, void * y);
void * node_copy(void * element);

/*mongodb part*/
const char *uri_string = "mongodb://admin:admin@localhost:27017";
   mongoc_uri_t *uri;
   mongoc_client_t *mongoclient;
   mongoc_database_t *database;
   mongoc_collection_t *collection;
   bson_t *insert, *extrainsert, *query, *opts, child, child2;
   bson_error_t error;
   char *str;
   bool retval;
   mongoc_cursor_t *cursor;
   const bson_t *doc;

   uint8_t sendbuffer[6];



void connmgr_listen(int port_number) {
  //variable declaration
  tcpsock_t * server, * client;
  int bytes, errorval;
  int conn_counter = 0;
  time_t servertime;
  int fd1;             //main connection fd.
  uint8_t sensorcounter = 0;
  dplist_t* gatewaynodes;		// make a list to store all the active connections
  dplist_t* sensornodes;		// make a list to store all the active connections
  sensor_data_buffer_t datatosendbuffer;
  FILE * fp_sensor;

  datatosendbuffer.datainbuffer = 0;
  mongoc_init ();
  /*
    * Safely create a MongoDB URI object from the given string
    */
  uri = mongoc_uri_new_with_error (uri_string, &error);
  if (!uri) {fprintf (stderr,
               "failed to parse URI: %s\n"
               "error message:       %s\n",
               uri_string,
               error.message);
   }

   /*
    * Create a new client instance
    */
   mongoclient = mongoc_client_new_from_uri (uri);
   if (!mongoclient) {
      printf("fault occurd");
   }

   /*
    * Register the application name so we can track it in the profile logs
    * on the server. This can also be done from the URI (see other examples).
    */
   mongoc_client_set_appname (mongoclient, "handrail-server");

   /*
    * Get a handle on the database "db_name" and collection "coll_name"
    */
   database = mongoc_client_get_database (mongoclient, "handrail");
   collection = mongoc_client_get_collection (mongoclient, "handrail", "sensordata");

  fp_sensor = fopen("sensor_data_recv","w");
  gatewaynodes = dpl_create(node_copy,node_free,node_compare);
  sensornodes = dpl_create(node_copy,node_free,node_compare);
  time(&servertime);

  if (tcp_passive_open( & server, port_number) != TCP_NO_ERROR) exit(EXIT_FAILURE);
  printf("Test server is started\n");
  //get the SD of the server in the fd1 variable
  tcp_get_sd(server, & fd1);

  /*build the query for fetching data specific for the server*/
  query = BCON_NEW ("handled", BCON_BOOL(false));
   BSON_APPEND_ARRAY_BEGIN (query, "$or", &child);
   BSON_APPEND_DOCUMENT_BEGIN (&child, "command", &child2);
   BSON_APPEND_INT32 (&child2, "command", STATUSREQ);
   bson_append_document_end (&child, &child2);
   BSON_APPEND_DOCUMENT_BEGIN (&child, "command", &child2);
   BSON_APPEND_INT32 (&child2, "command", REGISTEROK);
   bson_append_document_end (&child, &child2);
   BSON_APPEND_DOCUMENT_BEGIN (&child, "command", &child2);
   BSON_APPEND_INT32 (&child2, "command", GETTIMEOUTINT);
   bson_append_document_end (&child, &child2);
   BSON_APPEND_DOCUMENT_BEGIN (&child, "command", &child2);
   BSON_APPEND_INT32 (&child2, "command", SETTIMEOUTINT);
   bson_append_document_end (&child, &child2);
   BSON_APPEND_DOCUMENT_BEGIN (&child, "command", &child2);
   BSON_APPEND_INT32 (&child2, "command", TIMEOUTINT);
   bson_append_document_end (&child, &child2);
   BSON_APPEND_DOCUMENT_BEGIN (&child, "command", &child2);
   BSON_APPEND_INT32 (&child2, "command", BAT);
   bson_append_document_end (&child, &child2);
   BSON_APPEND_DOCUMENT_BEGIN (&child, "command", &child2);
   BSON_APPEND_INT32 (&child2, "command", GETLOWLIMIT);
   bson_append_document_end (&child, &child2);
   BSON_APPEND_DOCUMENT_BEGIN (&child, "command", &child2);
   BSON_APPEND_INT32 (&child2, "command", SETLOWLIMIT);
   bson_append_document_end (&child, &child2);
   BSON_APPEND_DOCUMENT_BEGIN (&child, "command", &child2);
   BSON_APPEND_INT32 (&child2, "command", GETHIGHLIMIT);
   bson_append_document_end (&child, &child2);
   BSON_APPEND_DOCUMENT_BEGIN (&child, "command", &child2);
   BSON_APPEND_INT32 (&child2, "command", GETAVG);
   bson_append_document_end (&child, &child2);
   BSON_APPEND_DOCUMENT_BEGIN (&child, "command", &child2);
   BSON_APPEND_INT32 (&child2, "command", SETHIGHLIMIT);
   bson_append_document_end (&child, &child2);
   BSON_APPEND_DOCUMENT_BEGIN (&child, "command", &child2);
   BSON_APPEND_INT32 (&child2, "command", SETAVG);
   bson_append_document_end (&child, &child2);
   BSON_APPEND_DOCUMENT_BEGIN (&child, "command", &child2);
   BSON_APPEND_INT32 (&child2, "command", GETMESINTERVAL);
   bson_append_document_end (&child, &child2);
   BSON_APPEND_DOCUMENT_BEGIN (&child, "command", &child2);
   BSON_APPEND_INT32 (&child2, "command", SETMESINTERVAL);
   bson_append_document_end (&child, &child2);
   BSON_APPEND_DOCUMENT_BEGIN (&child, "command", &child2);
   BSON_APPEND_INT32 (&child2, "command", SETBLUELED);
   bson_append_document_end (&child, &child2);
   BSON_APPEND_DOCUMENT_BEGIN (&child, "command", &child2);
   BSON_APPEND_INT32 (&child2, "command", RESETBLUELED);
   bson_append_document_end (&child, &child2);
   BSON_APPEND_DOCUMENT_BEGIN (&child, "command", &child2);
   BSON_APPEND_INT32 (&child2, "command", SETGREENLED);
   bson_append_document_end (&child, &child2);
   BSON_APPEND_DOCUMENT_BEGIN (&child, "command", &child2);
   BSON_APPEND_INT32 (&child2, "command", RESETGREENLED);
   bson_append_document_end (&child, &child2);
   BSON_APPEND_DOCUMENT_BEGIN (&child, "command", &child2);
   BSON_APPEND_INT32 (&child2, "command", SETREDLED);
   bson_append_document_end (&child, &child2);
   BSON_APPEND_DOCUMENT_BEGIN (&child, "command", &child2);
   BSON_APPEND_INT32 (&child2, "command", RESETREDLED);
   bson_append_document_end (&child, &child2);
   BSON_APPEND_DOCUMENT_BEGIN (&child, "command", &child2);
   BSON_APPEND_INT32 (&child2, "command", GOTOSLEEP);
   bson_append_document_end (&child, &child2);
   BSON_APPEND_DOCUMENT_BEGIN (&child, "command", &child2);
   BSON_APPEND_INT32 (&child2, "command", WAKEUP);
   bson_append_document_end (&child, &child2);
   bson_append_array_end (query, &child);

/*
    opts = BCON_NEW ("limit", BCON_INT64 (3));
    cursor = mongoc_collection_find_with_opts (collection, query, opts, NULL);
    while (mongoc_cursor_next (cursor, &doc)) 
    {
      sensor_data_t datatosend;
      bson_iter_t iter,baz;
      bson_t *update, *updatequery;
      const bson_oid_t *oid;
      if (bson_iter_init (&iter, doc) && bson_iter_find_descendant (&iter, "_id", &baz) && BSON_ITER_HOLDS_OID (&baz)) {
        oid = bson_iter_oid (&baz);
      }
      if (bson_iter_find_descendant (&iter, "nodeid", &baz) && BSON_ITER_HOLDS_INT32 (&baz)) {
        datatosend.id = bson_iter_int32 (&baz);
      }
      if(bson_iter_find_descendant (&iter, "command", &baz) && BSON_ITER_HOLDS_INT32 (&baz)) {
        datatosend.command = bson_iter_int32 (&baz);
      }
      if(bson_iter_find_descendant (&iter, "data", &baz) && BSON_ITER_HOLDS_INT32 (&baz)) {
       datatosend.data = bson_iter_int32 (&baz);
      }
                
      printf("received %d %d %d",datatosend.id,datatosend.command,datatosend.data);
      updatequery = BCON_NEW ("_id", BCON_OID (oid));       
      update = BCON_NEW ("$set", "{", "handled", BCON_BOOL (true),"}");
      if (!mongoc_collection_update (collection, MONGOC_UPDATE_NONE, updatequery, update, NULL, &error)) {
        printf ("%s\n", error.message);
      }
    }*/


  //infinite loop
  while (1) {
  	struct pollfd poll_arg[(1+conn_counter)];
    poll_arg[0].fd = fd1;
    poll_arg[0].events = POLLIN | POLLPRI;
    for(int i = 0;i < conn_counter;i++)
    {
    	node_info* node = (node_info*)dpl_get_element_at_index(gatewaynodes,i);
      	poll_arg[(i+1)].fd = node->socket->sd;
    	poll_arg[(i+1)].events = POLLIN | POLLPRI;
    }
    //check for data to send to one of the nodes!
    char *json;
    opts = BCON_NEW ("limit", BCON_INT64 (5));
    cursor = mongoc_collection_find_with_opts (collection, query, opts, NULL);
    while (mongoc_cursor_next (cursor, &doc)) 
    {
      json = bson_as_canonical_extended_json(doc,NULL);
      printf("%s\n",json);
      printf("goes through loop");
      bson_iter_t iter,baz;
      bson_t *update, *updatequery;
      const bson_oid_t *oid;
      if (bson_iter_init (&iter, doc) && bson_iter_find_descendant (&iter, "_id", &baz) && BSON_ITER_HOLDS_OID (&baz)) {
        oid = bson_iter_oid (&baz);
      }
      if (bson_iter_find_descendant (&iter, "nodeid", &baz) && BSON_ITER_HOLDS_INT32 (&baz)) {
        datatosendbuffer.sensor[datatosendbuffer.datainbuffer].id = bson_iter_int32 (&baz);
      }
      if(bson_iter_find_descendant (&iter, "command", &baz) && BSON_ITER_HOLDS_INT32 (&baz)) {
        //printf("command %d",bson_iter_int32 (&baz));
        datatosendbuffer.sensor[datatosendbuffer.datainbuffer].command = bson_iter_int32 (&baz);
      }
      if(bson_iter_find_descendant (&iter, "data", &baz) && BSON_ITER_HOLDS_INT32 (&baz)) {
       datatosendbuffer.sensor[datatosendbuffer.datainbuffer].data = bson_iter_int32 (&baz);
      }        
      printf("received %d %d %d",datatosendbuffer.sensor[datatosendbuffer.datainbuffer].id,datatosendbuffer.sensor[datatosendbuffer.datainbuffer].command,datatosendbuffer.sensor[datatosendbuffer.datainbuffer].data);

      if(datatosendbuffer.datainbuffer < 10) datatosendbuffer.datainbuffer++;

      updatequery = BCON_NEW ("_id", BCON_OID (oid));       
      update = BCON_NEW ("$set", "{", "handled", BCON_BOOL (true),"}");
      if (!mongoc_collection_update (collection, MONGOC_UPDATE_NONE, updatequery, update, NULL, &error)) {
        printf ("%s\n", error.message);
      }
    }
    //check if a new connection was made available
    poll(poll_arg, 1 + conn_counter, 500);
    if(poll_arg[0].revents == 1){
    	//if so add the node to the list
    	if (tcp_wait_for_connection(server, & client) != TCP_NO_ERROR) exit(EXIT_FAILURE);
      	printf("Incoming client connection\n");
      	//put the node info in the list.
		node_info* node = (node_info*)malloc(sizeof(node_info));
      	node->socket = client;
      	node->reading.id = 0;
      	node->reading.command = 0;
		    node->reading.data = 0;
      	dpl_insert_at_index(gatewaynodes,(void*)node,conn_counter,false);
      	conn_counter++;
    }
    else if(conn_counter > 0){
     	for(int i =0;i < conn_counter;i++)
      	{
      		//request the information of the node out the list
      		node_info* node = (node_info*)dpl_get_element_at_index(gatewaynodes,i);
    		if(poll_arg[(1+i)].revents >= 1)
    		{
    			//change the client information to archive the right info
      			client = node->socket;
      			//clear the errors 
      			errorval = TCP_NO_ERROR;

				bytes = sizeof(sensor_data_t);
        		errorval = tcp_receive(client, (void * ) &node->reading, & bytes,errorval);

        		switch(errorval)
        		{
        			//everything was OK so process the data that was received.
        			case TCP_NO_ERROR:
        				printf("sensor id = %" PRIu8 " - command = %" PRIu8 " - data = %d\n", node->reading.id, node->reading.command,node->reading.data); 
              insert = bson_new ();
              BSON_APPEND_INT32 (insert, "nodeid", node->reading.id);
              BSON_APPEND_INT32 (insert, "command", node->reading.command);
              switch (node->reading.command)
              {
              case REGISTER:
                  extrainsert = bson_new ();
                  BSON_APPEND_INT32 (extrainsert, "nodeid", node->reading.id);
                  BSON_APPEND_INT32 (extrainsert, "command", node->reading.command); 
                  BSON_APPEND_INT32 (extrainsert, "sleepmode", 0);
                  BSON_APPEND_INT32 (insert, "sensortype", node->reading.data);
                  if (!mongoc_collection_insert_one (collection, extrainsert, NULL, NULL, &error)) {
                  fprintf (stderr, "%s\n", error.message);
                  }
                break;
              case SHUTDOWN:
                  BSON_APPEND_INT32 (insert, "sleepmode", 1);
                break;
              case OK:
                  BSON_APPEND_INT32 (insert, "sleepmode", 0);
                break;
              case TIMEOUTINT:
                  BSON_APPEND_INT32 (insert, "timeoutinterval", node->reading.data);
                break;
              case BATPER:
                  BSON_APPEND_INT32 (insert, "battery", node->reading.data);
                break;
              case MODVAL:
                  BSON_APPEND_INT32 (insert, "lastenergy", node->reading.data);
                break;
              case GETLOWLIMIT:
                  BSON_APPEND_INT32 (insert, "lowerlimit", node->reading.data);
                break;
              case GETHIGHLIMIT:
                  BSON_APPEND_INT32 (insert, "highlimit", node->reading.data);
                break;
              case GETAVG:
                  BSON_APPEND_INT32 (insert, "avgvalue", node->reading.data);
                break;
              case GETMESINTERVAL:
                  BSON_APPEND_INT32 (insert, "measurementinterval", node->reading.data);
                break; 
              default:
                printf("invalid command sended");
                break;
              }

            //add data to the DB
            if (!mongoc_collection_insert_one (collection, insert, NULL, NULL, &error)) {
              fprintf (stderr, "%s\n", error.message);
            }
            
            if(node->reading.id == 11 && node->reading.command == REGISTER)
						{
              sensor_data_t datatosend;
							bytes = sizeof(datatosend);
              datatosend.id = 11;
              datatosend.command = BAT;
              datatosend.data = 0;
							tcp_send(client,(void *)&datatosend,&bytes);
						}
            uint8_t i = 0;
            while(i < datatosendbuffer.datainbuffer)
            {
							bytes = sizeof(datatosendbuffer.sensor[i]);
							tcp_send(client,(void *)&datatosendbuffer.sensor[i],&bytes);
              i++;
            }
            datatosendbuffer.datainbuffer = 0;
            /*
						sensornode_info* sensor = NULL;
						for(uint8_t i = 0;i < sensorcounter;i++){
							sensornode_info* tempnode = dpl_get_element_at_index(sensornodes,i);
							if(tempnode->nodeid == node->reading.id ){
								sensor = tempnode;
							}
							
						}
						if(sensor == NULL){
							sensor = (sensornode_info*)(malloc(sizeof(sensornode_info)));
							sensor->nodeid = node->reading.id;
							sensor->socket = node->socket;
							sensor->sleepmode = 0;
							sensor->sensortype = 0;
						}*/

        				break;
        			//the connection with the sensor was rejected so that we need to remove this sensor from the list
        			case TCP_CONNECTION_CLOSED:
        				printf("Sensor %d has closed the connection\r\n",node->reading.id);
        				if(tcp_close(&node->socket) == TCP_NO_ERROR)
    					{
        					free(node);
        					dpl_remove_at_index(gatewaynodes,i,false);
      						conn_counter--;
      						time(&servertime);
      					}
      					else
    					{
    						printf("a connection was rejected by the client but the server is unable to close the connection\r\n");
    					}
      					break;
      				//a socket error occurs so do nothing with the data.
      				case TCP_SOCKET_ERROR:
      					printf("socket error occured\r\n");
      					break;
      				//invalid command was send over the connection so ignore it for now.
      				case TCP_SOCKOP_ERROR:
      					printf("invalid socket operation was preformed\r\n");

        		}
    		}
      }
      //{ $or : [ { "command" : NumberInt(13) }, { "command" : NumberInt(14) }, { "command" : NumberInt(15) }, { "command" : NumberInt(16) }, { "command" : "" } ] }
      //query = BCON_NEW ("$or", "[", "{", "command", BCON_INT32 (13), "command", BCON_INT32 (14), "command", BCON_INT32 (15), "}", "]");
  	  //str = bson_as_canonical_extended_json (query, NULL);
      //printf ("%s\n", str);
    }
    else if(servertime < (time(NULL) - TIMEOUT))
    {
    if (tcp_close( & server) != TCP_NO_ERROR) exit(EXIT_FAILURE);
    	printf("No nodes found test server is shutting down\n");
    	dpl_free(&gatewaynodes,false);
    	fclose(fp_sensor);
      bson_destroy (insert);

      /*
      * Release our handles and clean up libmongoc
      */
      mongoc_collection_destroy (collection);
      mongoc_database_destroy (database);
      mongoc_uri_destroy (uri);
      mongoc_client_destroy (mongoclient);
      mongoc_cleanup ();
    	printf("exit\r\n");
	}
  }
}

void connmgr_free() 
{
	printf("nothing to free\n");
}

void * node_copy(void * element)
{
    return element;
}   

int node_compare(void * x, void * y)
{
	return -1;
}

void node_free(void ** element)
{
  	free(element);
} 

/*void datamgr_parse_sensor_data(FILE *fp_sensor_map)
{
	uint8_t sendbuffer[6] = {11,BAT,0,0,0,0};
    if(fp_sensor_map != NULL)
    {
        while(!feof(fp_sensor_map))
        {
            uint8_t sensorid, command;
			uint32_t data;
            sensor_average_t *avg_temp =  (sensor_average_t *)malloc(sizeof(sensor_avg_t));

            fscanf(fp_sensor_map, "%d %d %d\n", &sensorid, &command, &data);
            //send the data out of the file to the gateway
			sendbuffer[0] = sensorid;
			sendbuffer[1] = command;
			sendbuffer[2] = (data >> 24) & 0xFF;
			sendbuffer[3] = (data >> 16) & 0xFF;
			sendbuffer[4] = (data >> 8) & 0xFF;
			sendbuffer[5] = data & 0xFF;
        }
    }
}*/