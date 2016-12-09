MEMORY {
  rom (rx)  : ORIGIN = 0x00000000, LENGTH = 32K
  ram (rwx) : ORIGIN = 0x10000000, LENGTH =  4K
}

ENTRY( vector )

SECTIONS {
  . = 0x00000000;

  .text : {
    __head_text = .;
    *( .vector   )
    *( .handler  )
    *( .text*    )
    *( .text.*   )
    *( .rodata*  )
    *( .rodata.* )
    . = ALIGN( 4 );
    __foot_text = .;
  } >         rom

  .data         : ALIGN( 16 ) {
    __head_data = .;
    *( .data     )
    *( .data.*   )
    . = ALIGN( 4 );
    __foot_data = .;
  } > ram AT> rom

  .bss (NOLOAD) : ALIGN( 16 ) {
    __head_bss  = .;
    *( .bss      )
    *( .bss.*    )
    . = ALIGN( 4 );
    __foot_bss  = .;
  } > ram
}

PROVIDE( __load_text = LOADADDR( .text ) );
PROVIDE( __load_data = LOADADDR( .data ) );
PROVIDE( __load_bss  = LOADADDR( .bss  ) );

PROVIDE( __tos       = ORIGIN( ram ) + LENGTH( ram ) );
