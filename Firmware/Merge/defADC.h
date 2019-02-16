/**************************************************
* UART library with RTS/CTS flow support
*
* Author: Craig Cameron
* University of Strathclyde 2018
* Last edited: 27/04/18
 **************************************************/

#ifndef DEFADC_H
#define DEFADC_H

#include <msp430.h>
#include <driverlib.h>
#include <cstdio>
#include "defUART.h"

//----- Global Variable declarations -----
//(volatile as interrupt routine can change it unexpectedly)
//3300/1023 = 3.225 
extern volatile uint16_t ADCmV[4];// ADC measurment converterd to mV
extern volatile uint16_t ADCVT;
extern uint8_t counter; 

//----- Funciton prototype declarations ------
//void displayOnLCD(uint16_t num, unsigned char ch1, unsigned char ch2);
void configureADC();
void configurePINS();
void configureTimer();
void initADC();

void Y0();
void Y1();
void Y2();
void Y3();
#endif