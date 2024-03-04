#include <msp430fr6989.h>
#include <math.h>
#include <string.h>
#define FLAGS UCA1IFG
#define RXFLAG UCRXIFG
#define TXFLAG UCTXIFG
#define TXBUFFER UCA1TXBUF
#define RXBUFFER UCA1RXBUF
#define redLED BIT0
#define greenLED BIT7
#define BUT1 BIT1
#define BUT2 BIT2
void uart_write_char(unsigned char c);
unsigned char uart_read_char(void);
void Initialize_UART(void);
void Initialize_UART_2(void); //Use ACLK = 32768Hz, 4800 baud, no oversampling,
void uart_write_uint16(unsigned int n);
void uart_write_string(char * str);
void update_screen(void);
void config_ACLK_to_32KHz_crystal();
unsigned int request_runway1 = 0;
unsigned int forfeit_runway1 = 0;
unsigned int request_runway2 = 0;
unsigned int forfeit_runway2 = 0;
unsigned int runway1_in_use = 0;
unsigned int runway2_in_use = 0;
unsigned int change = 0; //If change on screen takes place
unsigned int runway1_inquiry = 0;
unsigned int runway2_inquiry = 0;
int main(void)
{
   WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer
    PM5CTL0 &= ~LOCKLPM5; //Enable GPIO Pins
    P1DIR |= redLED;
    P9DIR |= greenLED;
    P1DIR &= ~(BUT1 | BUT2); //Set as input configuration
    P1REN |= (BUT1 | BUT2); //Enable pull up resistor
    P1OUT |= (BUT1 | BUT2); //Active low using pull-up resistor
    P1IFG &= ~(BUT1 | BUT2); //Clear button interrupts before calling it
    P1IE |= (BUT1 | BUT2);
    P1OUT &= ~redLED;
    P9OUT &= ~greenLED;
    Initialize_UART();
    config_ACLK_to_32KHz_crystal();
    unsigned int x = 0;
    unsigned char data;
    uart_write_string("\n\rORLANDO EXECUTIVE AIRPORT RUNWAY CONTROL\n\r\n\r\n\r\t\tRunway 1\tRunway 2\n\rRequest(RQ):\t1\t\t\t3\n\rForfeit(FF):\t7\t\t\t9\n\r\n\r\n\r\n\r\n\r-----\t-----\n\rRUNWAY1\tRUNWAY2\n\r-----\t-----\n\r");
    TA0CCR0 = 40000; //
    TA0CCTL0 |= CCIE;
    TA0CCTL0 &= ~CCIFG;
    TA0CTL |= TASSEL_1 | ID_0 | MC_0 | TACLR; //ACLK, No input divider, Up mode, be off initially, clear register
    __enable_interrupt();
    for(;;){
        data = uart_read_char();
        switch(data){
            case '1':
                if(request_runway1) break; //Already requested
                change = 1;
                request_runway1 = 1;
                P1OUT |= redLED;
                break;
            case '7':
                if(!runway1_in_use) break; //If runway not in use, can't forfeit it
                change = 1;
                forfeit_runway1 = 1;
                runway1_in_use = 0;
                runway1_inquiry = 0;
                P1OUT &= ~redLED;
                TA0CTL &= ~MC_3; //Clear mode field, turn off timer
                break;
            case '3':
                if(request_runway2) break; //Already requested
                change = 1;
                request_runway2 = 1;
                P9OUT |= greenLED;
                break;
            case '9':
                if(!runway2_in_use) break; //If runway not in use, can't forfeit it
                change = 1;
                forfeit_runway2 = 1;
                runway2_inquiry = 0;
                runway2_in_use = 0;
                P9OUT &= ~greenLED;
                TA0CTL &= ~MC_3; //Clear mode field, turn off timer
                break;
            default:
                //change = 0;
                break;
        }
        if(change){
            update_screen();
            if(forfeit_runway1) forfeit_runway1 = 0;
            if(forfeit_runway2) forfeit_runway2 = 0;
            if(runway1_inquiry) runway1_inquiry = 0;
            if(runway2_inquiry) runway2_inquiry = 0;
            change = 0;
        }
    }
}
void update_screen(void){
    // \x1b[1A - Move cursor up a line
    //
    if(request_runway1){
                uart_write_string("\r"); //Put cursor back to beginning of current line
                uart_write_string("Requested");
            }
            if(request_runway2)
            {
                uart_write_string("\r");
                    uart_write_string("\x1b[10C"); //Move cursor to the right 5 columns
                    uart_write_string("Requested");
            }
    uart_write_string("\r");
    if(runway1_inquiry){
        uart_write_string("\n\r");
        uart_write_string("\n\r");
        uart_write_string("\n\r");
        uart_write_string("Inquiry");
    }
    if(runway2_inquiry){
        uart_write_string("\n\r");
        uart_write_string("\n\r");
        uart_write_string("\n\r");
        uart_write_string("\x1b[10C"); //Move cursor to the right 5 column
        uart_write_string("Inquiry");
    }
    if(runway1_inquiry | runway2_inquiry){
        uart_write_string("\x1b[3A"); //Up three lines
        uart_write_string("\r");
        return;
    }
    if(runway1_in_use){
           uart_write_string("\n\r");
           uart_write_string("In use");
           uart_write_string("\r");
        }
    if(runway2_in_use){
            uart_write_string("\n\r");
            uart_write_string("\x1b[10C"); //Move cursor to the right 5 column
            uart_write_string("In use");
            uart_write_string("\r");
    }
        //Put cursor back at top left
   if(runway1_in_use | runway2_in_use)
       {
           uart_write_string("\x1b[1A"); //Up one line
           return;
       }
    //Delete "Requested" text, "In use" text, and Inquiry text if forfeitting
    if(forfeit_runway1){
        //Delete "Requested" Line
        uart_write_string("\x1b[9C"); //Move cursor to the right 5 columns
        uart_write_string("\x1b[1K"); //Delete from start of line to cursor
        //uart_write_string("\r");
        uart_write_string("\x1b[1B"); //Move down 1 line
        uart_write_string("\x1b[1K"); //Delete from start of line to cursor
        uart_write_string("\x1b[2B"); //Move down 2 lines
        uart_write_string("\x1b[1K"); //Delete from start of line to cursor
        uart_write_string("\x1b[3A"); //Up three lines
        uart_write_string("\r");
    }
    if(forfeit_runway2){
        //Delete "Requested" Line
        uart_write_string("\x1b[10C"); //Move cursor to the right 5 columns
        uart_write_string("\x1b[0K"); //Delete from cursor to end of line
        uart_write_string("\x1b[1B"); //Move down 1 line
        uart_write_string("\x1b[0K"); //Delete from cursor to end of line
        uart_write_string("\x1b[2B"); //Move down 2 lines
        uart_write_string("\x1b[0K"); //Delete from cursor to end of line
        uart_write_string("\x1b[3A"); //Up three lines
        uart_write_string("\r"); //Back
    }
    if(forfeit_runway1 | forfeit_runway2) return;
    uart_write_string("\r");
}
void uart_write_char(unsigned char c){
    while( (FLAGS & TXFLAG) == 0) {} //Wait until transfer flag is set (wait's for an on-going transmission to finish)
    TXBUFFER = c;
    return;
}
unsigned char uart_read_char(void){
    if( (FLAGS & RXFLAG) == 0) return 0;
    unsigned char temp = RXBUFFER;
    return temp;
}
void uart_write_uint16(unsigned int n){
    //Parse the integer into it's ASCII values
    unsigned int num_digits = (n == 0) ? 1 : log10(n) + 1;
    unsigned int digits[5] = {0}; //No more than 5 digits needed for 16 bit value
    int x = (num_digits-1);
    int temp = n;
    while(x>=0){
        digits[x] = temp % 10;
        temp /= 10;
        x--;
    }
    int i;
    for(i = 0; i<num_digits;i++){
        uart_write_char(48 + digits[i]);
    }
}
void uart_write_string(char * str){
    unsigned int length = strlen(str);
    int i;
    for(i = 0; i<length; i++){
        uart_write_char(str[i]);
    }
}
// Configure UART to the popular configuration
// 9600 baud, 8-bit data, LSB first, no parity bits, 1 stop bit
// no flow control, oversampling reception
// Clock: SMCLK @ 1 MHz (1,000,000 Hz)
void Initialize_UART(void){
    // Configure pins to UART functionality
    P3SEL1 &= ~(BIT4|BIT5);
    P3SEL0 |= (BIT4|BIT5);
    // Main configuration register
    UCA1CTLW0 = UCSWRST;    // Engage reset; change all the fields to zero
    // Most fields in this register, when set to zero, correspond to the
    // popular configuration
    UCA1CTLW0 |= UCSSEL_2;  // Set clock to SMCLK
   // Configure the clock dividers and modulators (and enable oversampling)
    UCA1BRW = 6;                // divider
    // Modulators: UCBRF = 8 = 1000 --> UCBRF3 (bit #3)
    // UCBRS = 0x20 = 0010 0000 = UCBRS5 (bit #5)
    UCA1MCTLW = UCBRF3 | UCBRS5 | UCOS16;
    // Exit the reset state
    UCA1CTLW0 &=  ~UCSWRST;
}
void Initialize_UART_2(void){ //Use ACLK = 32768Hz, 4800 baud, no oversampling.
        // Configure pins to UART functionality
        P3SEL1 &= ~(BIT4|BIT5);
        P3SEL0 |= (BIT4|BIT5);
        // Main configuration register
        UCA1CTLW0 = UCSWRST;    // Engage reset; change all the fields to zero
        UCA1CTLW0 |= UCSSEL_1; //Select ACLK
        UCA1BRW = 6; //Still a 6 divider
        /*Modulators for 32khZ clock, 4800 baud rate
         *UCOS16 =0
         *UCBRx = 6
         *UCBRFx = -
         *UCBRSx = 0xEE
         */
        UCA1MCTLW &= ~0x00; //Clear field
        UCA1MCTLW = 0xEE << 8; //Shift UCBRXs to top 8 bits.
        // Exit the reset state
        UCA1CTLW0 &=  ~UCSWRST;
}
//**********************************
// Configures ACLK to 32 KHz crystal
void config_ACLK_to_32KHz_crystal() {
// By default, ACLK runs on LFMODCLK at 5MHz/128 = 39 KHz
// Reroute pins to LFXIN/LFXOUT functionality
PJSEL1 &= ~BIT4;
PJSEL0 |= BIT4;
// Wait until the oscillator fault flags remain cleared
CSCTL0 = CSKEY; // Unlock CS registers
do {
CSCTL5 &= ~LFXTOFFG; // Local fault flag
SFRIFG1 &= ~OFIFG; // Global fault flag
} while((CSCTL5 & LFXTOFFG) != 0);
CSCTL0_H = 0; // Lock CS registers
return;
}
#pragma vector = TIMER0_A0_VECTOR
__interrupt void T0A0_ISR(){ //Only used to flash the LED's
    if(runway1_in_use) P1OUT ^= redLED;
    else if(runway2_in_use) P9OUT ^= greenLED;
    //Don't need to clear the flag here
    return;
}
#pragma vector = PORT1_VECTOR
__interrupt void Port1_ISR(){
    if( (P1IFG & BUT1) == BUT1){
        if(runway2_in_use) {
            P1IFG &= ~(BUT1 | BUT2);
            return;
        } // Runway can't be in use because runway 1 is being used
        if(runway1_in_use){ //If already in use, it is an inquiry
                    runway1_inquiry = 1;
                    change = 1;
                    P1IFG &= ~(BUT1 | BUT2);
                    return;
                }
        if(!request_runway1) { //Has to be requested to be in use
            P1IFG &= ~(BUT1 | BUT2);
            return;
        } //Has to be requested to be in use
            change = 1;
            request_runway1 = 0; //Request granted
            TA0CTL |= MC_1; //Turn on timer to up mode for flashing LED
            runway1_in_use = 1;
    }
    if( (P1IFG & BUT2) == BUT2){
        if(runway1_in_use) {
            runway1_inquiry = 1;
            P1IFG &= ~(BUT1 | BUT2);
            return;
        } //Runway 1 being used, runway 2 can't be in use
        if(runway2_in_use)
                {
                            runway2_inquiry = 1;
                            change = 1;
                            P1IFG &= ~(BUT1 | BUT2);
                            return;
                } //If already in use, it is an inquiry
        else if(!request_runway2) {
            P1IFG &= ~(BUT1 | BUT2);
            return;
        } //Has to be requested to be in use
            change = 1;
            request_runway2 = 0; //Request granted
            TA0CTL |= MC_1; //Turn on timer to up mode for flashing LED
            runway2_in_use = 1;
        }
    __delay_cycles(60000); //Debouncing
    P1IFG &= ~(BUT1 | BUT2);
    return;
}
