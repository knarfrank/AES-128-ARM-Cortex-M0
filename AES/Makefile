 INCLUDES  = . 

 SOURCES   = ./target.c 
 SOURCES  += ./target.S

 HEADERS   = ./target.h 

 SCRIPTS   = ./target.ld

 TARGETS   = ./target.elf
 TARGETS  += ./target.map
 TARGETS  += ./target.bin
 TARGETS  += ./target.hex

 CC_PATHS  = $(patsubst %, -I %, ${INCLUDES})
 CC_FLAGS  = -Wall -std=gnu99 -Os $(patsubst %, -T %, ${SCRIPTS}) -mcpu=cortex-m0 -mthumb -nostartfiles 
 CC_LIBS   = 

%.elf %.map : ${SOURCES} ${HEADERS} ${SCRIPTS}
	@arm-none-eabi-gcc ${CC_PATHS} ${CC_FLAGS} -Wl,-Map=${*}.map -o ${*}.elf ${SOURCES} ${CC_LIBS}
%.bin       : %.elf
	@arm-none-eabi-objcopy --gap-fill=0 -O binary ${<} ${@}
%.hex       : %.elf
	@arm-none-eabi-objcopy --gap-fill=0 -O ihex   ${<} ${@}

all      : ${TARGETS}

clean    :
	@rm -rf ${TARGETS}

spotless : clean

program  : ${TARGETS}
	@lpc21isp -hex $(filter %.hex, ${^}) ${USB} 9600 12000
