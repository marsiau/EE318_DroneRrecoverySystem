/**************************************************
* UART library with RTS/CTS flow support
*
* Author: Marius Siauciulis
* University of Strathclyde 2018
*
* Fargo Maestro 100 Lite RS-232 settings:
*    o 9600bps
*    o 8 data bits,
*    o no parity,
*    o 1 stop bit,
*    o hardware flow control (CTS/RTS)
* References:
* msp430fr413x_euscia0_uart_03.c
**************************************************/
#ifndef defUART_h
#define defUART_h

#include <msp430.h>
#include <stdint.h>                             //For uintX_t
#include <stdbool.h>                            //For bool

//Define maximum message size for Rx
#define MAX_MSG_SIZE 160

//----- Structure declarations -----
enum statusFlags{STOP, CONT, PAUSE};
//----- Structure declarations -----
struct UARTMsgStruct
{
  enum statusFlags status;
  char *pdata;                                  //Pointer to char msg data
  uint8_t len;
  uint8_t i;
}extern TxMsg, RxMsg;

//----- Variable declarations -----
extern char RXData;                    //Rx message data
//Variable signaling whether HFC is enabled/disabled (RTS/CTS lines)
extern bool HFC_flag;                           //Hardware Flow Controll Flag

//----- Function declarations -----
void init_UART_GPIO();                          //Initialize UART GPIO
void init_UART();                               //Initialize UART
void enable_HFC();                              //Enable Hardware Flow Controll
void disable_HFC();                             //Disable Hardware Flow Controll
bool send_over_UART(char *pdata, uint8_t lenght);//Send msg over UART
#endif

/*
Notes:
uintX_t and boolean type variables are used as they:
  o document the intent for type/range of values stored 
  o are efficiant
  o personal preference
enum type for flags is created for convieniance
*/