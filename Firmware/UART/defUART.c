#include "defUART.h"

//----- Variable definitions -----
struct TxMsgStruct TxMsg;
char RXData[40];
unsigned int iRx = 0;
bool HFC_flag;
bool temp_flag = false;


//----- Interupt rutine for UART -----
#pragma vector=USCI_A0_VECTOR
__interrupt void USCI_A0_ISR(void)
{
    switch(__even_in_range(UCA0IV,USCI_UART_UCTXCPTIFG))
    {
        case USCI_NONE: break;
        case USCI_UART_UCRXIFG: 
          RXData[iRx] = UCA0RXBUF;
          iRx++;
          if(iRx > 40)
          {
            temp_flag = true;
            UCA0IE &= ~UCRXIE;//Disable USCI_A0 RX interrupt
          }
          break;
        case USCI_UART_UCTXIFG: 
          if(TxMsg.iTx == TxMsg.len - 1)
          {
            UCA0IE &= ~UCTXIE;                  //Disable USCI_A0 TX interrupt
            TxMsg.sending = false;
          }
          else
          {
            TxMsg.iTx++;
            UCA0TXBUF = *(TxMsg.pdata + TxMsg.iTx);
          }
          break;
        case USCI_UART_UCSTTIFG: break;
        case USCI_UART_UCTXCPTIFG: break;
    }
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
      
    }
    else                                        //CTS went LOW
    {
      P2IES  &=  ~BIT7;                         //Interrupt on low-to-high transition
      if(TxMsg.sending)
      {
        //Start/Resume transmission
        UCA0TXBUF = TxMsg.pdata[TxMsg.iTx];     //Load/reload data onto buffer
        UCA0IE |= UCTXIE;                       //Enable USCI_A0 TX interrupt
      }
    }   
    P2IE   |=   BIT7;                           //Interrupt reenabled
    P2IFG   &=  ~BIT7;                          //Interrupt flag cleared
    break;
  }
}
//----- Function definitions -----
void init_UART_GPIO()
{
  // Configure UART Rx/Tx pins
  P1SEL0 |= BIT0 | BIT1;                        //Set 2-UART pin as second function
  // Configure UART RTS/CTS pins
  //RTS
  P2SEL0 |= BIT5;                               //P2.5(RTS) as output
  //CTS
  P2SEL0 &=  ~BIT7;                             //P2.7(CTS) as input
  P2REN  |=   BIT7;                             //Enable pull up/down resistor 
  P2OUT  |=   BIT7;                           //Enable pull Up
  //P2OUT  &=   ~BIT7;                            //Enable pull Down
  
  P8SEL0 |= BIT0; 
  P8OUT &= ~BIT0;
}

void init_UART()
{
  init_UART_GPIO();
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
  TxMsg.sending = false;
  TxMsg.pdata   = 0;
  TxMsg.len     = 0;
  TxMsg.iTx     = 0;
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
  P2OUT  &= ~BIT5;                              //P2.5 - off, RTS - on
}
void disable_HFC()                              //Disable Hardware Flow Controll
{
  HFC_flag = false;
  P2IE &= ~BIT7;                                //CTS interrupt disabled
  P2OUT  != BIT5;                               //P2.5 - on, RTS - off
}
bool send_over_UART(char *pdata, uint8_t lenght)
{
  if(TxMsg.sending)
    return false;                               //Other message is being sent
  else
  {
    if(HFC_flag)
    { 
      P2OUT &= ~BIT5;                              //P2.5 - off, RTS - on
    }
    //Prepare data
    TxMsg.sending = true;
    TxMsg.pdata   = pdata;
    TxMsg.len     = lenght;
    TxMsg.iTx     = 0;
    //if(!(HFC_flag && !(P2IN & BIT7)))              //If CTS is on - start sending
    //{
      UCA0IE |= UCTXIE;                         //Enable USCI_A0 TX interrupt
      UCA0TXBUF = TxMsg.pdata[TxMsg.iTx];       //Load data onto buffer
    //}
    //else CTS interrupt will start transmission when CTS goes high
    return true;
  }
 }
    