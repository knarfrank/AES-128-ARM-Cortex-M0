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

uint8_t target_uart_rd(void) {
  while( !( UART->U0LSR & ( 0x1 << 0 ) ) ) { /* spin */ }
  return (UART->U0RBR);
}

void target_uart_wr(uint8_t x) {
  while( !( UART->U0LSR & ( 0x1 << 5 ) ) ) { /* spin */ }
        ( UART->U0THR = x );
}

void target_led_trig(bool x) {
  if(x) {
    GPIO0->GPIOnDATA |=  ( 0x1 << 8 );
  } else {
    GPIO0->GPIOnDATA &= ~( 0x1 << 8 );
  }
}

void target_led_user( bool x ) {
  if(x) {
    GPIO0->GPIOnDATA |=  ( 0x1 << 9 );
  } else {
    GPIO0->GPIOnDATA &= ~( 0x1 << 9 );
  }
}

void print(char* string, int n) {
    for(int i = 0; i < n; i++) {
	target_uart_wr((char)string[i]);
    }
    target_uart_wr((char)'\n');
}

#define NUM_ROUNDS 10
// Calculate number of bytes for a key based on number of rounds
#define NUM_BYTES (128 + ((NUM_ROUNDS-10)*32))/8

static const uint8_t sbox[256] =   {
  //0     1    2      3     4    5     6     7      8    9     A      B    C     D     E     F
  0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
  0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
  0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
  0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
  0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
  0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
  0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
  0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
  0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
  0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
  0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
  0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
  0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
  0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
  0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
  0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16 
};

static const uint8_t rsbox[256] =
{ 0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb,
  0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb,
  0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
  0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25,
  0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92,
  0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
  0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06,
  0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b,
  0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
  0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e,
  0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b,
  0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
  0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f,
  0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef,
  0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
  0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d 
};

void addRoundKey() {

}

void subBytes() {

}

void shiftRows() {

}

void mixColumns() {

}

void encrypt(char m[NUM_BYTES], char c[NUM_BYTES], char k[NUM_BYTES]) {
    addRoundKey();
    for(int r = 0; r < NUM_ROUNDS; r++) {
	subBytes();
        shiftRows();
        mixColumns();
        addRoundKey();
    }
    // Final 'round' without mix columns
    subBytes();
    shiftRows();
    addRoundKey();
}

int main( int argc, char* argv[] ) {
  target_init();

  char plainText[NUM_BYTES];
  char cipherText[NUM_BYTES];
  char key[NUM_BYTES];

  while( 1 ) {
    encrypt(plainText, cipherText, key);
    
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