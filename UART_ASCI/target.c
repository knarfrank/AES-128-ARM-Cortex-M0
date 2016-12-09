#include "target.h"

uint8_t target_reg_k[ 16 ];
uint8_t target_reg_m[ 16 ];
uint8_t target_reg_c[ 16 ];

SYSCON_t* SYSCON = ( SYSCON_t* )( 0x40048000 );
 IOCON_t*  IOCON = (  IOCON_t* )( 0x40044000 );
  GPIO_t*  GPIO0 = (   GPIO_t* )( 0x50000000 );
  GPIO_t*  GPIO1 = (   GPIO_t* )( 0x50010000 );
  GPIO_t*  GPIO2 = (   GPIO_t* )( 0x50020000 );
  GPIO_t*  GPIO3 = (   GPIO_t* )( 0x50030000 );
  UART_t*   UART = (   UART_t* )( 0x40008000 );

void target_init() {
  SYSCON->SYSAHBCLKCTRL |=  (   1 <<  6 ); // Table  21: enable GPIO  clock
  SYSCON->SYSAHBCLKCTRL |=  (   1 << 12 ); // Table  21: enable UART  clock
  SYSCON->SYSAHBCLKCTRL |=  (   1 << 16 ); // Table  21: enable IOCON clock

  // Section 13.5.15, set baud rate
  //
  // rate = clk / ( 16 * ( 256 * U0DLM + U0DLL ) * ( 1 + ( DivAddVal / MulVal ) ) )
  // 
  // st. for a 12 MHz clock frequency we want, per Table 201, to set
  // DivAddVal = 0, MulVal = 1, U0DLM = 0 and U0DLL = 78.

  SYSCON->UARTCLKDIV     =  1;

  UART->U0LCR           |=  ( 0x1 <<  7 ); // Table 193:  enable divisor latch access
  UART->U0FDR            =  ( 0x0 <<  0 ); // Table 200: set DivAddVal
  UART->U0FDR           |=  ( 0x1 <<  4 ); // Table 200: set    MulVal
  UART->U0DLM            =  0;             // Table 187: set divisor latch MSBs
  UART->U0DLL            = 78;             // Table 187: set divisor latch LSBs
  UART->U0LCR           &= ~( 0x1 <<  7 ); // Table 193: disable divisor latch access

  UART->U0LCR           |=  ( 0x3 <<  0 ); // Table 193: 8 data bits
  UART->U0LCR           &= ~( 0x1 <<  3 ); // Table 193: no parity
  UART->U0LCR           &= ~( 0x1 <<  2 ); // Table 193: 1 stop bits

  IOCON->PIO0_8          =  ( 0x0 <<  0 ); // Table  80: configure PIO0_8 as GPIO
  IOCON->PIO0_9          =  ( 0x0 <<  0 ); // Table  81: configure PIO0_9 as GPIO
  IOCON->PIO1_6          =  ( 0x1 <<  0 ); // Table  97: configure PIO1_6 as UART RxD
  IOCON->PIO1_7          =  ( 0x1 <<  0 ); // Table  98: configure PIO1_7 as UART TxD

  GPIO0->GPIOnDIR        =  ( 0x1 <<  8 ); // Table 175: PIO0_8 => trigger
  GPIO0->GPIOnDATA      &= ~( 0x1 <<  8 );
  GPIO0->GPIOnDIR       |=  ( 0x1 <<  9 ); // Table 175: PIO0_9 => user
  GPIO0->GPIOnDATA      &= ~( 0x1 <<  9 );
}

uint8_t target_uart_rd( void      ) {
  while( !( UART->U0LSR & ( 0x1 << 0 ) ) ) { /* spin */ }
  return ( UART->U0RBR     );
}

void    target_uart_wr( uint8_t x ) {
  while( !( UART->U0LSR & ( 0x1 << 5 ) ) ) { /* spin */ }
        ( UART->U0THR = x );
}

void    target_led_trig( bool x ) {
  if( x ) {
    GPIO0->GPIOnDATA |=  ( 0x1 << 8 );
  }
  else {
    GPIO0->GPIOnDATA &= ~( 0x1 << 8 );
  }
}

void    target_led_user( bool x ) {
  if( x ) {
    GPIO0->GPIOnDATA |=  ( 0x1 << 9 );
  }
  else {
    GPIO0->GPIOnDATA &= ~( 0x1 << 9 );
  }
}

int main( int argc, char* argv[] ) {
  target_init();

  while( 1 ) {
    // print ASCII table

    for( int i = 0; i <     256; i++ ) {
      target_uart_wr( ( char )( i ) );
    }

    // wait

    for( int i = 0; i < 1000000; i++ ) {
      asm( "nop" );
    }

    // turn off LEDs

    target_led_trig( 0 );
    target_led_user( 0 );

    // wait

    for( int i = 0; i < 1000000; i++ ) {
      asm( "nop" );
    }

    // turn on  LEDs

    target_led_trig( 1 );
    target_led_user( 1 );

  }

  return 0;
}
