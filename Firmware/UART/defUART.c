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
#include "defUART.h"

//----- Variable definitions -----
struct UARTMsgStruct TxMsg, RxMsg;
bool HFC_flag = false;

//-------------------- Interupt handlers --------------------//
//----- Interupt rutine for UART -----
#pragma vector=USCI_A0_VECTOR
__interrupt void USCI_A0_ISR(void)
{
    switch(__even_in_range(UCA0IV,USCI_UART_UCTXCPTIFG))
    {
        case USCI_NONE: break;
        case USCI_UART_UCRXIFG:                 //Rx interrupt
          TA1CTL |= MC_0; TA1R = 0;             //Disable/reset the timer
          RxMsg.data[RxMsg.i] = UCA0RXBUF;
          RxMsg.i++;
          TA1CTL |= MC_1;                       //Enable the timer
          TA1CCTL0 |= CCIE;//CCR0 interrupt enabled
          break;
        case USCI_UART_UCTXIFG:                 //Tx interrupt
          if(TxMsg.i == TxMsg.len - 1)
          {
            UCA0IE &= ~UCTXIE;                  //Disable USCI_A0 TX interrupt
            TxMsg.status = STOP;
          }
          else
          {
            TxMsg.i++;
            UCA0TXBUF = TxMsg.data[TxMsg.i];
          }
          break;
        case USCI_UART_UCSTTIFG: break;
        case USCI_UART_UCTXCPTIFG: break;
    }
}

//----- Interrupt handler for timer-----
#pragma vector = TIMER1_A0_VECTOR
__interrupt void TIMERA1_ISR(void) 
{
  TA1CTL |= MC_0; TA1R = 0;             //Disable/reset the timer
  TA1CCTL0 &= ~CCIE;//CCR0 interrupt disabled
  parse_msg(RxMsg.data);               //Parse the received data
  RxMsg.i =0;                           //Reset i to override the msg
  RxMsg.status = STOP;
}
//----- Interupt rutine for GPIO CTS implementation-----
#pragma vector = PORT2_VECTOR
__interrupt void P2_interrupt_handler(void)
{
  switch(__even_in_range(P2IV,P1IV_P1IFG7))//Checks all pins on P2
  {
  case P2IV_P2IFG7: //P2.7
    P2IE   &=   ~BIT7;                          //Disable interrupt
    if(P2IN & BIT7)                             //CTS went HIGH
    {  
      P2IES  |=   BIT7;                         //Interrupt on high-to-low transition
      //Pause\Stop transmittion 
      UCA0IE &= ~UCTXIE;                        //Disable USCI_A0 TX interrupt 
      TxMsg.status = PAUSE; 
    }
    else                                        //CTS went LOW
    {
      P2IES  &=  ~BIT7;                         //Interrupt on low-to-high transition
      if(TxMsg.status == PAUSE)
      {
        //Start/Resume transmission
        UCA0TXBUF = TxMsg.data[TxMsg.i];     //Load/reload data onto buffer
        UCA0IE |= UCTXIE;                       //Enable USCI_A0 TX interrupt
        TxMsg.status = CONT; 
        P4OUT  &= ~BIT0;                        //Turn off P4.0
      }
    }   
    P2IE   |=   BIT7;                           //Interrupt reenabled
    P2IFG   &=  ~BIT7;                          //Interrupt flag cleared
    break;
  }
}
//-------------------- Function definitions --------------------//
void init_UART_GPIO()
{
  // Configure UART Rx/Tx pins
  P1SEL0 |= BIT0 | BIT1;                        //Set 2-UART pin as second function
  // Configure UART RTS/CTS pins
    //RTS
  P8SEL0 |= BIT0;                               //P8.0 (RTS) as output
    //CTS
  P2SEL0 &=  ~BIT7;                             //P2.7(CTS) as input
  P2REN  |=   BIT7;                             //Enable pull up/down resistor 
  //P2OUT  |=   BIT7;                             //Enable pull Up
  P2OUT  &=   ~BIT7;                            //Enable pull Down
  //MUX pins
  
    //LED
  P4DIR |= BIT0;                                //P4.0 as output
  P4OUT &= ~BIT0;                               //Turn P4.0 off
}

void init_Rx_Timer()
{
  //ACLK = 32768Hz
  TA1CTL |= TACLR;// Clear the timer.
  //ACLK as clock, timer turned off, 
  TA1CTL |= TASSEL_1 | MC_0;
  TA1R = 0;// Write inital counter value
  TA1CCR0 = 0xFFFF;//Count to max to get ~0.5s
  //TA1CCTL0 |= CCIE;//CCR0 interrupt enabled
  TA1CCTL0 &= ~CCIE;//CCR0 interrupt disabled
}

void init_UART()
{
  init_UART_GPIO();
  init_Rx_Timer();
  // Configure UART
  UCA0CTLW0 |= UCSWRST;                         //Software RST enabled, Put eUSCI in reset state
  //eUSCI_A clock source select. These bits select the BRCLK source clock.
  UCA0CTLW0 |= UCSSEL__SMCLK;                   //eUSCI_A clock source is SMCLK               
  /* Baud Rate calculation
    1000000/9600 = 104.166 >> 16 - Using the oversampling mode
    -> OS16 = 1, 
    -> UCBRx = INT(N/16) = INT(104.166/16) = INT(6.510) = 6
    -> UCBRFx = INT([(N/16) - INT(N/16)]*16) = INT([6.51 - 0.51]*16) = 96, makes no sense from table = 8
    -> From Table 15-4 UCBRSx = 0x20
  */
  UCA0BR0 = 6;                              
  UCA0MCTLW |= UCOS16;                          //Enable oversampling mode (Table15-5)
  UCA0MCTLW |= 0x2080;                          //0x20 + 8                                        
  UCA0BR1 = 0;
  UCA0CTLW0 &= ~UCSWRST;                        //Initialize eUSCI
 
  UCA0IE |= UCRXIE;                           // Enable USCI_A0 RX interrupt
  
  //Initialize TxMsg variable
  TxMsg.status = STOP;
  TxMsg.len     = 0;
  TxMsg.i       = 0;
  //Initialize RxMsg variable
  RxMsg.status = STOP;
  RxMsg.len     = 0;
  RxMsg.i       = 0;
}

void enable_HFC()                              //Enable Hardware Flow Controll
{
  HFC_flag = true;
  if(P2IN & BIT7)                              //Check the initial state of CTS
  {
    P2IES  |=   BIT7;                          //Interrupt on high-to-low transition
  }
  else
  {
    P2IES  &=  ~BIT7;                           //Interrupt on low-to-high transition
  }
  P2IE   |=   BIT7;                             //CTS interrupt enabled
  P2IFG   &=  ~BIT7;                            //Interrupt flag cleared
  P8OUT &= ~BIT0;                               //P8.0 - off, RTS - on
}
void disable_HFC()                              //Disable Hardware Flow Controll
{
  HFC_flag = false;
  P2IE &= ~BIT7;                                //CTS interrupt disabled
  P8OUT |= BIT0;                               //P8.0 - on, RTS - off
}
bool send_over_UART(char data[], uint8_t lenght)
{
  if(TxMsg.status != STOP)
  {
    return false;                               //Other message is being sent
  }
  else
  {
    if(HFC_flag)
    { 
      P8OUT &= ~BIT0;                               //P8.0 - off, RTS - on
    }
    //Prepare data
    TxMsg.status = CONT;
    strcpy(TxMsg.data, data);
    TxMsg.len     = lenght;
    TxMsg.i       = 0;
    if(HFC_flag && (P2IN & BIT7))               //If HFC enabled, CTS - off
    {
      //CTS interrupt will start transmission when CTS goes high
      TxMsg.status = PAUSE; 
      P4OUT |= BIT0;                           //Turn P4.0
    }
    else
    {
      UCA0IE |= UCTXIE;                         //Enable USCI_A0 TX interrupt
      UCA0TXBUF = TxMsg.data[TxMsg.i];       //Load data onto buffer
    }
    return true;
  }
 }

void parse_msg(char msgData[])                //Parse received data
{
  /*
  
rv                              //Returns voltage of the cells
rloc                            //Returns location
setvt                           //Sets voltage threshold
setnr                           //Sets phone number
bon                             //Buzzer on
boff                            //Buzzer 
  */
  if(strstr(msgData, "OK") != NULL)
  {
    displayScrollText("OK");
  }
  //Include the other commands
  else
  {
    displayScrollText("NOP");
  }
}