#include "uart.h"
#include "native_io.h"

volatile uint32_t* pios_aux_reg = (volatile uint32_t*) PBASE + AUX_BASE_ADDR;
void dummy ( unsigned int );


// TODO: Gpio.s
#define GPFSEL1         (PBASE+0x00200004)
#define GPSET0          (PBASE+0x0020001C)
#define GPCLR0          (PBASE+0x00200028)
#define GPPUD           (PBASE+0x00200094)
#define GPPUDCLK0       (PBASE+0x00200098)

void pios_uart_init ( )
{    
    // set the GPIO-pins into the correct state
    gpio_pinmode ( 14, 2 );
    gpio_pinmode ( 15, 2 );
    
    pios_aux_reg[AUX_ENABLE] = AUX_UART;
    pios_aux_reg[AUX_MU_CNTL] = 0;
    pios_aux_reg[AUX_MU_IER] = 0;
    pios_aux_reg[AUX_MU_LCR] = 3;
    pios_aux_reg[AUX_MU_MCR] = 0;
    pios_aux_reg[AUX_MU_IER] = 0;
    pios_aux_reg[AUX_MU_IIR] = 0xc6;
    pios_aux_reg[AUX_MU_LCR] = 1;   // 8 bit mode
    pios_aux_reg[AUX_MU_BAUD] = 270;
                
    /**
     * set-up of Pull Up / Down: 
       *    1. Write to GPPUD to set the required control signal (i.e. Pull-up or Pull-Down or neither to remove the current Pull-up/down)
       *    2. Wait 150 cycles – this provides the required set-up time for the control signal
       *    3. Write to GPPUDCLK0/1 to clock the control signal into the GPIO pads you wish to modify – NOTE only the pads which receive a clock will be modified, all others will retain their previous state.
       *    4. Wait 150 cycles – this provides the required hold time for the control signal
       *    5. Write to GPPUD to remove the control signal
       *    6. Write to GPPUDCLK0/1 to remove the clock
       * see: Manual pg 101
    **/
    unsigned ra;
    ra=(*(uint32_t*)GPFSEL1);
    ra&=~(7<<12); //gpio14
    ra|=2<<12;    //alt5
    ra&=~(7<<15); //gpio15
    ra|=2<<15;    //alt5
    (*(uint32_t*)GPFSEL1)=ra;
    (*(uint32_t*)GPPUD)=0;
    for(ra=0;ra<150;ra++) { dummy(ra); };
    (*(uint32_t*)GPPUDCLK0) = (1<<14)|(1<<15);
    for(ra=0;ra<150;ra++) { dummy(ra); };
    (*(uint32_t*)GPPUDCLK0) = 0;
    
    pios_aux_reg[AUX_MU_CNTL] = 3;
}

void pios_uart_puts ( const char* str )
{
    while ( *str )
    {
        pios_uart_putchar(*str);
        str++;
    }
}

void pios_uart_write ( const char* str, size_t len )
{
    for ( int i=0; i<len; i++ )
    {
        pios_uart_putchar ( str[i] );
    }
}
void pios_uart_read ( char* buff, size_t len )
{
    for ( int i=0; i<len; i++ )
    {
        buff[i] = (char) pios_uart_getchar();
    }
}

void pios_uart_putchar ( const char c )
{
    while ( 1 ) 
    {
        if ((pios_aux_reg[AUX_MU_LSR] & AUX_TX_EMPTY) != 0)
            break;
    }
    pios_aux_reg[AUX_MU_IO] = ((0xff) & c);
}

uint32_t pios_uart_getchar ( )
{
    while ( 1 ) 
    {
        if ((pios_aux_reg[AUX_MU_LSR] & AUX_RX_DATA) != 0 ) 
            break;
    }
    return (0xff & pios_aux_reg[AUX_MU_IO]);
}

void pios_uart_setBaud ( uint16_t baudfactor )
{
    pios_aux_reg[AUX_MU_BAUD] = baudfactor;
}

void pios_uart_setDataSize ( int size )
{
    uint32_t val = pios_aux_reg[AUX_MU_LCR];
    val = (val & 0xfffffffe) | ((size == 8) ? 1 : 0);   // 8 bit mode
    pios_aux_reg[AUX_MU_LCR] = val;
}

bool pios_uart_rxReady ()
{
    return ((pios_aux_reg[AUX_MU_LSR] & AUX_RX_DATA) == 0 );
}

bool pios_uart_txReady ()
{
    return ((pios_aux_reg[AUX_MU_LSR] & AUX_TX_EMPTY) == 0 );
}

int pios_uart_rxQueue ()
{
    return (((0x0f000000) & pios_aux_reg[AUX_MU_STAT]) >> 24);
}
int pios_uart_txQueue ()
{
    return (((0x000f0000) & pios_aux_reg[AUX_MU_STAT]) >> 16);
}


