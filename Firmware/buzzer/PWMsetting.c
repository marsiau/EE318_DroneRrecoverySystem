#include <msp430.h>
#include <driverlib.h>

void TA0PwmSetPeriod(unsigned int TimerA0_period)
{   // Basics: See User Guide 11.2.2 for how to set the timer.  
    // Step 1: Clear the timer.
    TA0CTL = TACLR; 
    // Step 2: Write the initial counter value.
    TA0R=0; // just zero.
    
    // Step 3: Initialize TA0CCR0
    //TA0CCR0 : Capture-Compare register. 
    //This is the register whose value is compared to the TA0R Counter Register Value.
    TA0CCR0 = TimerA0_period; //Timer_A0 Capture/compare register ¨C 
    // put in the count-to value. 
    // Note: This setting, combined with the MC_1 mode will make the timer 
    //to run up to this value and reset.
    
    // Step 4: Apply desired configuration to TA0IV, TAIDEX and TA0CCTLn
    // TA0IV: Nothing to write here - all bits are read-only
    
    // TA0EX0: Timer_A expansion register. 
    // See User Guide 11.3.6 and msp430fr4133.h line 2338
    // TA0EX0 left as default - no divider expansion.   
    
    // TA0CCTL0: Timer_A0 Capture/compare Control 0.  
    //See User Guide 11.3.3 and msp430fr4133.h line 23014
    TA0CCTL1 &= ~(CAP); // make sure the timer is in COMPARE mode (CAP bit reset)
    TA0CCTL1 |= OUTMOD_3; //TEnable the interrupt when compare event happens
    
    // Step 5: Apply desired configuration to TA0CTL, including MC bits. 
    // (this means that MC bits are important)    
    // TA0CTL: Timer_A0 Control. See User Guide 11.3.1 and msp430fr4133.h line 2261
    // settings here: clock source = ACLK, no clock division, 
    // mode: count up to CCR0, and enable overall interrupt generation
    TA0CTL |= TASSEL_1 | ID_0 | MC_1 | TAIE;    
    // Note: ACLK is the slow, 32KHz clock. For clock types, see User Guide 3.1;     
} // end setup_timer_method_1


 void TA0PwmSetPermill(unsigned int Percent)
{
    unsigned long int Period;
    unsigned int Duty;
    Period = TA0CCR0;
    Duty = Period * Percent / 100;
    
    TA0CCR1= Duty;
}
