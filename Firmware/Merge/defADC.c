t /**************************************************
* UART library with RTS/CTS flow support
*
* Author: Craig Cameron
* University of Strathclyde 2018
* Last edited: 27/04/18
 **************************************************/

#include "defADC.h"

//----- Global Variables -----
volatile uint16_t ADCmV[4];             // ADC measurment converterd to mV
volatile uint16_t ADCVT = 0;
uint8_t counter = 0;

//----- Interupt rutine for ADC -----
//ADCMEM0 ranges 0 - 1023
//----- Interupt rutine for ADC -----
//ADCMEM0 ranges 0 - 1023
#pragma vector=ADC_VECTOR
__interrupt void ADC_ISR(void)
{
  switch(__even_in_range(ADCIV,ADCIV_ADCIFG))
  {
  case ADCIV_ADCIFG:
    //3300/1023 = 3.225 
    
    //ADCmV is 16bit thus 
    //1st most significant digit has to be discarded
  
      if (counter==0)
      {  
        Y3();
        ADCmV[0] = (int)(ADCMEM0*4.719465);
        counter=counter+1;
        Y0();
      }
      else if (counter==1)
      {  
        Y0();
        ADCmV[1] = (int)(ADCMEM0*4.719465);
        counter=counter+1;
        Y1();
      }
      else if (counter==2)
      { 
        Y1();
        ADCmV[2] = (int)(ADCMEM0*4.719465);
        counter=counter+1;
        Y2();
      }
      else if (counter==3)
      {  
        Y2();
        ADCmV[3] = (int)(ADCMEM0*4.719465);
        ADCVT= ADCmV[0] + ADCmV[1] + ADCmV[2] + ADCmV[3];
        strcat(temp_msg, "ADC ");
        snprintf(temp_msg, POLLED_MSG_SIZE, "%u", ADCVT);
        strcat(temp_msg, "MV");
        strcat(polled_msg, temp_msg);
        memset(temp_msg, 0, MAX_MSG_SIZE);
        counter=0;
        Y3();
      }
    ADCIFG &= ~(ADCIFG0);                       // Clear interrupt flag
    break;
  }
}
//-------------------- Function definitions --------------------//
void initADC()
{
  configureADC();
  configurePINS();
  configureTimer(); 
}

void configureADC()
{
  //----- Configure ADC -----
  SYSCFG2 |= ADCPCTL9;                  //Configure pin 9 as ADC in
  //0010b = 16 ADCCLK cycles | | turn on ADC
  ADCCTL0 |= ADCSHT_2  | ADCON;         // 16 ADCCLK cycles | ADC on
  //TA1.1 trigger | SAMPCON triggered by sampling timer | 10b = Repeat-single-channel
  ADCCTL1 |= ADCSHS_2 | ADCSHP | ADCCONSEQ_2;
  //0b = 8 bit (10 clock cycle conversion time)
  ADCCTL2 |= ADCRES;            
  
  //Configure ADC mux
  ADCMCTL0 |= ADCINCH_9;                // 1001b = A9, 000b = Vr+ = AVCC and Vr- = AVSS
  //Configure the intrerrupt
  ADCIFG &= ~(0x01);                    // Clear interrupt flag
  ADCIE |= ADCIE0;                      // Enable ADC conversion complete interrup
}

void configurePINS()
{
  //pins for multiplexer
  
  GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN3);
  GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN4);
  GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN5);
}
void configureTimer()
{
  //----- Setup TA1.1 for 8Hz ADC driver -----7
  //Configure ADC timer trigger TA1.1
  //The count-to value, 32768/8 = 4096 = 0x1000
  TA1CTL = TACLR;                       // Clear the timer.
  TA1CCR0 =  0x1000;                    // Reset every 0x1000 tics
  TA1CCR1 =  0x800;                     // Toggle OUT every 0x800 to turn the ADC on 8 times/s
  TA1CCTL1 = OUTMOD_7;                  // TA1CCR1 toggle
  TA1CTL |= TASSEL_1 | MC_1;             // ACLK |up mode    
}

//MUX
void Y0()
{
      GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN3);
      GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN4);  
      GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN5);  
}
void Y1()
{
      GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN3);
      GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN4);  
      GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN5);  
}       
void Y2()
{
      GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN3);
      GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN4);  
      GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN5);  
}
 void Y3()
{
      GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN3);
      GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN4);  
      GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN5);  
}
//S0 - p1.3
//S1 - p1.4
//S2 - p1.5