/*
 * File:   nrf24l01.c
 * Author: pic18fxx.blogspot.com
 */
#include <xc.h>
#include <stdio.h>
#include "nrf24l01.h"
#include "time_delay.h"
#include "mcc_generated_files/spi1.h"
#include "mcc_generated_files/pin_manager.h"

// This data type sets the address data pipe 0.

void NRF24L01_WriteRegister(unsigned char Mnemonic, unsigned char value){
NRF24L01_CSN = 0;
SPI1_Exchange8bit(W_REGISTER | Mnemonic);
SPI1_Exchange8bit(value);
NRF24L01_CSN = 1;       
}

unsigned char NRF24L01_ReadRegister(unsigned char Mnemonic){
unsigned char byte0;
NRF24L01_CSN = 0; 
SPI1_Exchange8bit(R_REGISTER | Mnemonic);
byte0 = SPI1_Exchange8bit(0x00);
NRF24L01_CSN = 1; 
return byte0;
}

void NRF24L01_WriteBuffer(unsigned char data, unsigned char *buffer, unsigned char bytes){
unsigned char i;
NRF24L01_CSN = 0;                                    
SPI1_Exchange8bit(data);                       
for(i = 0; i < bytes; i++)     
   {
    SPI1_Exchange8bit(*buffer);
    buffer++;
   }
NRF24L01_CSN = 1;  
}

void NRF24L01_ReadBuffer(unsigned char data, unsigned char *buffer, unsigned char bytes){
unsigned char i;
NRF24L01_CSN = 0;                                    
SPI1_Exchange8bit(data);                       
for(i = 0; i < bytes; i++)     
   {
    *buffer = SPI1_Exchange8bit(0x00);
    buffer++;
   }
*buffer = NULL;
NRF24L01_CSN = 1;  
}

void NRF24L01_Init(unsigned char mode, unsigned char rf_channel,unsigned char *ADDRESS_DATA_PIPE0,unsigned char *ADDRESS_DATA_PIPE1){
NRF24L01_TRIS_CSN = 0; NRF24L01_TRIS_CE = 0;
NRF24L01_CSN = 1; NRF24L01_CE = 0;
Delay_ms(100);  
NRF24L01_WriteRegister(CONFIG, 0x0A); Delay_ms(10);
NRF24L01_WriteRegister(EN_AA, 0x03);
NRF24L01_WriteRegister(EN_RXADDR, 0x03);
NRF24L01_WriteRegister(SETUP_AW, 0x03);
NRF24L01_WriteRegister(SETUP_RETR, 0x38);
NRF24L01_SetChannel(rf_channel);
NRF24L01_WriteRegister(RF_SETUP, 0x06);
//NRF24L01_WriteRegister(STATUSS, 0x70);
NRF24L01_WriteBuffer(W_REGISTER | RX_ADDR_P0, ADDRESS_DATA_PIPE0, 5);
NRF24L01_WriteBuffer(W_REGISTER | RX_ADDR_P1, ADDRESS_DATA_PIPE1, 5);
NRF24L01_WriteBuffer(W_REGISTER | TX_ADDR, ADDRESS_DATA_PIPE0, 5); 
NRF24L01_WriteRegister(RX_PW_P0, PAYLOAD_BYTES); 
NRF24L01_WriteRegister(RX_PW_P1, PAYLOAD_BYTES); 
//NRF24L01_Flush();
Delay_ms(100);
NRF24L01_SetMode(mode);
Delay_ms(100);
}

void NRF24L01_SetMode(unsigned char mode){
NRF24L01_Flush();
NRF24L01_WriteRegister(STATUSS, 0x70); // Clear STATUS.    
switch(mode)
      {
       case 1:
              NRF24L01_WriteRegister(CONFIG, 0x0F);  // RX Control
              NRF24L01_CE = 1;
       break;
       case 2:
              NRF24L01_WriteRegister(CONFIG, 0x0E);  // TX Control 
              NRF24L01_CE = 0;
       break;
      }
}

void NRF24L01_SendData(unsigned char *buffer){
NRF24L01_SetMode(TX_MODE);    
NRF24L01_WriteBuffer(W_TX_PAYLOAD, buffer, PAYLOAD_BYTES);
NRF24L01_CE = 1; Delay_ms(1); NRF24L01_CE = 0;
}

unsigned char Nrf24_rxFifoEmpty(void)
{
	unsigned char fifoStatus;
	fifoStatus = NRF24L01_ReadRegister(FIFO_STATUS);
	return (fifoStatus & (0x01));
}

unsigned char NRF24L01_DataReady(void){
if((NRF24L01_ReadRegister(STATUSS) & 0x40) == 0x40)
  {
   return 1; 
  }
return !Nrf24_rxFifoEmpty();
}

void NRF24L01_ReadData(unsigned char *buffer){
NRF24L01_ReadBuffer(R_RX_PAYLOAD, buffer, PAYLOAD_BYTES);
NRF24L01_WriteRegister(STATUSS, 0x70); // Clear STATUS.
}

void NRF24L01_SetChannel(unsigned char rf_channel){
NRF24L01_WriteRegister(RF_CH, rf_channel);
}

unsigned char NRF24L01_GetChannel(void){
return NRF24L01_ReadRegister(RF_CH);
}

void NRF24L01_StandbyI(void){   
NRF24L01_WriteRegister(CONFIG, 0x0A);
Delay_ms(10);
}

void NRF24L01_Flush(void){
NRF24L01_CSN = 0; 
SPI1_Exchange8bit(FLUSH_TX);
NRF24L01_CSN = 1; 
NRF24L01_CSN = 0; 
SPI1_Exchange8bit(FLUSH_RX);
NRF24L01_CSN = 1; 
}