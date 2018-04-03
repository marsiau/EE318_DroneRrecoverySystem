


#include <msp430.h>
#include <driverlib.h>
#include "main.h"

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;                         // Stop WDT
     PMM_unlockLPM5();
      __enable_interrupt(); 
      
     P4SEL0 |= BIT1 | BIT2; 
     P4OUT = 0x00;
     P4DIR = 0xFF;
  
	do
	{
		CSCTL7 &= ~(XT1OFFG | DCOFFG);      // Clear XT1 and DCO fault flag
		SFRIFG1 &= ~OFIFG;
	} while (SFRIFG1 & OFIFG);              // Test oscillator fault flag

	CSCTL3 |= SELREF__XT1CLK;               // Set XT1CLK as FLL reference source
	CSCTL1 &= ~(DCORSEL_7);                 // Clear DCO frequency select bits first
	CSCTL1 |= DCORSEL_3;                    // Set DCO = 8MHz
	CSCTL2 = FLLD_0 + 243;                  // DCODIV = 8MHz

	do
	{
		__delay_cycles(7 * 31 * 8);         // Requires 7 reference clock delay before
											// polling FLLUNLOCK bits
											// @8 MHz, ~1736 cycles
	} while(CSCTL7 & (FLLUNLOCK0 | FLLUNLOCK1));// Poll until FLL is locked

	CSCTL4 = SELMS__DCOCLKDIV | SELA__XT1CLK;  // Set ACLK = XT1CLK = 32768Hz
												// DCOCLK = MCLK and SMCLK source
	CSCTL5 |= DIVM_0 | DIVS_0;              // MCLK = DCOCLK = 8MHZ,
		  									// SMCLK = MCLK = 8MHz

        
        
           P1SEL0 |= BIT7;         
           P1DIR |= BIT7;  
        
      TA0PwmSetPeriod(500);  //500 ACLK clock cycle 
      TA0PwmSetPermill(20); // 20% high level voltage from P1.7;
    GPIO_setAsOutputPin(GPIO_PORT_P4, GPIO_PIN0);
  
   GPIO_setOutputHighOnPin(GPIO_PORT_P4, GPIO_PIN0);
    while(1);
   
}

