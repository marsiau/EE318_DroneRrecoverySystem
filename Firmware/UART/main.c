/**************************************************
* UART testing
*
* Author: Marius Siauciulis
* University of Strathclyde 2018
*
**************************************************/
#include <msp430.h>
#include <driverlib.h>
#include "defUART.h"
#include "hal_LCD.h"

char temp_msg[POLLED_MSG_SIZE];
char TestMsg[] = {"AT+WOPEN=2\r\n"};

//----- Interupt rutine for GPIO -----
#pragma vector = PORT1_VECTOR
__interrupt void P1_interrupt_handler(void)
{
  switch(__even_in_range(P1IV,P1IV_P1IFG7))//Checks all pins on P1
  {
  case P1IV_P1IFG2:                             //PIN2 - SW1
    __delay_cycles(10000);                      //Simple debauncing
    if(!(P1IN & BIT2))
    {
      send_over_UART(TestMsg, sizeof(TestMsg)-1);
    }
    break;
  }
}

int main(void)
{
  WDTCTL = WDTPW | WDTHOLD;                 // Stop watchdog timer
  PM5CTL0 &= ~LOCKLPM5;                     // Disable the GPIO power-on default high-impedance mode
                                            // to activate previously configured port settings
  Init_LCD();         //for debugging
  init_UART(); 
  sel_GPS();
  //sel_GSM();
  //disable_HFC();
  //enable_HFC();
  //Configure GPIO pins
    //Button
  P1DIR &= ~BIT2; //P1.2 as input
  P1REN |= BIT2; //Enable pull up/down resistor on P1.2
  P1OUT |= BIT2; //Select pull up
  P1IES |= BIT2; ////Interrupt on high-to-low transition
  P1IE  |= BIT2; //Interrupt enabled
  P1IFG &= ~BIT2; // P1.2 interrupt flag cleared      

  displayScrollText("HELLO");
  clearLCD();
  
  //Enable interrupts
  __enable_interrupt();
  
  while (1)
  {
    if(polled_msg[0] != '\0')
    {
      /*
        displayScrollText() requires a lot of cycles to complete and polled_msg
        might be updated in the meantime
      */
      strcpy(temp_msg, polled_msg);
      memset(polled_msg, 0, sizeof(polled_msg));    //Clean the memory
      displayScrollText(temp_msg);                       
      clearLCD();
    }
    else
    {
      __bis_SR_register(LPM3_bits + GIE);     // Enter LPM3 w/interrupt 
      __no_operation();                       // For debugger
    }
  }
}

