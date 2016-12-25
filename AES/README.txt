- the LPC114FN28 is wired up as follows

                       +--------------------+
  trigger = PIO0_8 =  1|                    |28
     user = PIO0_9 =  2|                    |27             
            SWCLK  =  3|                    |26             
                      4|                    |25             
                      5|                    |24 = ISP
                      6|                    |23 = RST       
                      7| [A] LPC114FN28/102 |22 = GND       
                      8|                    |21 = VCC = 3.3v
                      9|                    |20             
                     10|                    |19             
                     11|                    |18             
            SWDIO  = 12|                    |17             
                     13|                    |16 = TxD       
                     14|                    |15 = RxD
                       +--------------------+

- you'll need the following software

  1. a bare-metal ARM compiler tool-chain, i.e., arm-none-eabi GCC,
     and
  2. the programmer lpc2isp,

  both of which are available as packages in Ubuntu: just do

  sudo apt-get install lpc21isp gcc-arm-none-eabi

  the UART chip on the board is a standard FTDI part; I don't think
  I needed any extra udev rules, but it'd be worth making sure this
  is the case once you plug it in: just execute dmesg and check the
  new device is allocated a device node, e.g., /dev/USB0.

- you need a linker script (included), and a trampoline that copies
  data from flash into RAM (included) before invoking main.  beyond
  that it's possible to just write C programs; the example includes
 
  - some boiler-plate code to initialise and use the UART and LEDs,
  - a main function that a) prints an ASCII table to the UART (9600 
    8/N/1) and blinks the user and trigger LEDs.

  most of it is commented wrt. the datasheet (e.g., memory map) in

  http://www.nxp.com/documents/data_sheet/LPC111X.pdf

- the chip has a bootloader that allows programming over USB; the 
  idea is you 

  1. compile the image 
  2. invoke lpc21isp
  3. press and hold reset   button
  4. press and hold program button
  5. release        reset   button
  6. lpc2isp downloads program
  7. release        program button.

  The Makefile automates this: you can just do something like

  USB=/dev/USB0 make clean all program

- It's *possible* to hook the board up to a debugger via SWD; I've 
  got one, but *if* you want to go down this route, it'd be worth 
  us getting hold of another to use.
