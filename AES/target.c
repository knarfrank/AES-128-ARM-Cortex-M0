

#include <stdint.h>
#include "target.h"

#if !ARM_BOARD
    #include <stdio.h>
#endif



#if ARM_BOARD
uint8_t target_reg_k[ 16 ];
uint8_t target_reg_m[ 16 ];
uint8_t target_reg_c[ 16 ];

SYSCON_t* SYSCON = (SYSCON_t*)(0x40048000);
IOCON_t* IOCON   = (IOCON_t*)(0x40044000);
GPIO_t* GPIO0    = (GPIO_t*)(0x50000000);
GPIO_t* GPIO1    = (GPIO_t*)(0x50010000);
GPIO_t* GPIO2    = (GPIO_t*)(0x50020000);
GPIO_t* GPIO3    = (GPIO_t*)(0x50030000);
UART_t* UART     = (UART_t*)(0x40008000);

void target_init() {
    SYSCON->SYSAHBCLKCTRL |= (1 <<  6); // Table  21: enable GPIO  clock
    SYSCON->SYSAHBCLKCTRL |= (1 << 12); // Table  21: enable UART  clock
    SYSCON->SYSAHBCLKCTRL |= (1 << 16); // Table  21: enable IOCON clock

    // Section 13.5.15, set baud rate
    //
    // rate = clk / ( 16 * ( 256 * U0DLM + U0DLL ) * ( 1 + ( DivAddVal / MulVal ) ) )
    //
    // st. for a 12 MHz clock frequency we want, per Table 201, to set
    // DivAddVal = 0, MulVal = 1, U0DLM = 0 and U0DLL = 78.
    SYSCON->UARTCLKDIV =  1;

    UART->U0LCR       |=  (0x1 << 7); // Table 193:  enable divisor latch access
    UART->U0FDR        =  (0x0 << 0); // Table 200: set DivAddVal
    UART->U0FDR       |=  (0x1 << 4); // Table 200: set    MulVal
    UART->U0DLM        =  0;          // Table 187: set divisor latch MSBs
    UART->U0DLL        =  78;         // Table 187: set divisor latch LSBs
    UART->U0LCR       &= ~(0x1 << 7); // Table 193: disable divisor latch access

    UART->U0LCR       |=  (0x3 << 0); // Table 193: 8 data bits
    UART->U0LCR       &= ~(0x1 << 3); // Table 193: no parity
    UART->U0LCR       &= ~(0x1 << 2); // Table 193: 1 stop bits

    IOCON->PIO0_8      =  (0x0 << 0); // Table  80: configure PIO0_8 as GPIO
    IOCON->PIO0_9      =  (0x0 << 0); // Table  81: configure PIO0_9 as GPIO
    IOCON->PIO1_6      =  (0x1 << 0); // Table  97: configure PIO1_6 as UART RxD
    IOCON->PIO1_7      =  (0x1 << 0); // Table  98: configure PIO1_7 as UART TxD

    GPIO0->GPIOnDIR    =  (0x1 << 8); // Table 175: PIO0_8 => trigger
    GPIO0->GPIOnDATA  &= ~(0x1 << 8);
    GPIO0->GPIOnDIR   |=  (0x1 << 9); // Table 175: PIO0_9 => user
    GPIO0->GPIOnDATA  &= ~(0x1 << 9);
}

uint8_t target_uart_rd(void) {
    while(!(UART->U0LSR & (0x1 << 0))) {
        /* spin */
    }
    return (UART->U0RBR);
}

void target_uart_wr(uint8_t x) {
    while(!(UART->U0LSR & (0x1 << 5))) {
        /* spin */
    }
    ( UART->U0THR = x );
}

void target_led_trig(bool x) {
    if(x) {
        GPIO0->GPIOnDATA |=  (0x1 << 8);
    } else {
        GPIO0->GPIOnDATA &= ~(0x1 << 8);
    }
}

void target_led_user(bool x) {
    if(x) {
        GPIO0->GPIOnDATA |=  (0x1 << 9);
    } else {
        GPIO0->GPIOnDATA &= ~(0x1 << 9);
    }
}
#endif

void print(char* string, int n) {
    #if ARM_BOARD
        for(int i = 0; i < n; i++) {
            target_uart_wr((char)string[i]);
        }
        target_uart_wr((char)'\n');
    #else
        printf("%s\n", string);
    #endif
}

typedef uint8_t state_t[16];
static state_t* state;

// The lookup-tables are marked const so they can be placed in read-only storage instead of RAM
// The numbers below can be computed dynamically trading ROM for RAM -
// This can be useful in (embedded) bootloader applications, where ROM is often limited.
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


static void ShiftRows(void) {
    uint8_t temp;

    // Rotate first row 1 columns to left
    temp         = (*state)[1];
    (*state)[1]  = (*state)[5];
    (*state)[5]  = (*state)[9];
    (*state)[9]  = (*state)[13];
    (*state)[13] = temp;

    // Rotate second row 2 columns to left
    temp         = (*state)[2];
    (*state)[2]  = (*state)[10];
    (*state)[10] = temp;

    temp         = (*state)[6];
    (*state)[6]  = (*state)[14];
    (*state)[14] = temp;

    // Rotate third row 3 columns to left
    temp         = (*state)[3];
    (*state)[3]  = (*state)[15];
    (*state)[15] = (*state)[11];
    (*state)[11] = (*state)[7];
    (*state)[7]  = temp;
}

static uint8_t xtime(uint8_t x) {
    return ((x<<1) ^ (((x>>7) & 1) * 0x1b));
}

// MixColumns function mixes the columns of the state matrix
static void MixColumns(void) {
    uint8_t i;
    uint8_t Tmp,Tm,t;
    for(i = 0; i < 16; i += 4) {
        t   = (*state)[i];
        Tmp = (*state)[i] ^ (*state)[i+1] ^ (*state)[i+2] ^ (*state)[i+3] ;
        Tm  = (*state)[i] ^ (*state)[i+1];
        Tm = xtime(Tm);
        (*state)[i+0] ^= Tm ^ Tmp;

        Tm  = (*state)[i+1] ^ (*state)[i+2];
        Tm = xtime(Tm);
        (*state)[i+1] ^= Tm ^ Tmp;

        Tm  = (*state)[i+2] ^ (*state)[i+3];
        Tm = xtime(Tm);
        (*state)[i+2] ^= Tm ^ Tmp;

        Tm  = (*state)[i+3] ^ t;
        Tm = xtime(Tm);
        (*state)[i+3] ^= Tm ^ Tmp;
    }
}

// Cipher is the main function that encrypts the PlainText.
static void Cipher(uint8_t* key) {
    uint8_t round = 0;
    uint8_t i;
    uint8_t rcon = 0x01;

    for(round = 0; round < 10; ++round) {
        // Add Key
        for(i = 0; i < 16; ++i) {
            (*state)[i] ^= key[i];
        }
        // SubBytes
        for(i = 0; i < 16; ++i) {
            (*state)[i] = sbox[(*state)[i]];
        }

        ShiftRows();

        if(round < 9) {
            MixColumns();
        }

        //key schedule
        // compute the 16 next round key bytes
        key[0] = sbox[key[13]] ^ key[0] ^ rcon;
        key[1] = sbox[key[14]] ^ key[1];
        key[2] = sbox[key[15]] ^ key[2];
        key[3] = sbox[key[12]] ^ key[3];
        for (i=4; i<16; i++) {
            key[i] = key[i] ^ key[i-4];
        }
        // compute the next Rcon value
        rcon = xtime(rcon);
    }

    for(i=0; i<16; ++i) {
        (*state)[i] ^= key[i];
    }
}
static void BlockCopy(uint8_t* output, uint8_t* input) {
  uint8_t i;
  for (i = 0; i < 16; ++i) {
    output[i] = input[i];
  }
}



void AES128_ECB_encrypt(uint8_t* input, uint8_t* key, uint8_t* output) {
    // Copy input to output, and work in-memory on output
    BlockCopy(output, input);
    state = (state_t*)output;

    Cipher(key);
}

static void test_encrypt_ecb(void) {
    uint8_t key[] = {0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c};
    uint8_t in[]  = {0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96, 0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a};
    uint8_t out[] = {0x3a, 0xd7, 0x7b, 0xb4, 0x0d, 0x7a, 0x36, 0x60, 0xa8, 0x9e, 0xca, 0xf3, 0x24, 0x66, 0xef, 0x97};
    uint8_t buffer[16];

    AES128_ECB_encrypt(in, key, buffer);

    if(0 == memcmp((char*) out, (char*) buffer, 16)) {
        print("SUCCESS!", 8);
    } else {
        print("FAILURE!", 8);
    }
}

int main( int argc, char* argv[] ) {

#if ARM_BOARD
    target_init();
#endif
    while(1) {
    	print("Encrypting...", 13);
    	test_encrypt_ecb();

#if ARM_BOARD

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

#endif
    }
    return 0;
}
