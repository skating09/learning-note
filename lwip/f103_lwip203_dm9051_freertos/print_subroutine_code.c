
#include "main.h"
//#include "stm32f4x7_eth.h"
#include <string.h>
#include "System_Init.h"
#include "DM9051.h"

/**
 * Concentrate the RX debug display printing...
 * Trigger-counter
 */
static unsigned long show_Itask_c; // = 0;	
/**
 * Acc-counter
 */
static int show_inval_c = 0; //=ethernetif_show_inval_begin(void)
static int show_rxb1_c = 0, p_inpackt_count= 0;
static int show_rxb0_c = 0, p_idle_count= 0;

void ethernetif_show_inval_begin(void)
{
	show_inval_c = 0; // begin to show 'n' times inval
}

void ethernetif_print_opt_recovery(void)
{
		if (show_inval_c)
		{
			show_inval_c = 0;
			printf("*************************************\r\n");
			printf("*** DM9051 inval show_count.clear (RX recovery-OK) ***\r\n");
			printf("*************************************\r\n");
		}
}

int ethernetif_print_opt_err_rxbyte(uint8_t rxbyte)
{
	int ret = 0;
	if (show_inval_c < 3){
		printf("\r\n");
		if (show_inval_c==0) {
			printf("*******************************************\r\n");
			printf("*** DM9051 inval show_count.error.found (Explicity, Reset the RX FIFO pointer ) ***\r\n");
			printf("*******************************************\r\n");
			ret = 1; //statisic_explicity_rxreset_proc++;
		}
		/* Check to continue working ability ... */
		printf("inval show_count= %d\r\n", show_inval_c);
		printf("rxbyte = %02X.\r\n", rxbyte);
		printf("DM9051 rx: rx error, stop device \r\n");
		show_inval_c++;
	}
	return ret;
}

int ethernetif_printdebug_count_rxbyte_0x01(void) /* valid pkt count */
{
	show_rxb1_c++;
			
	if (!(show_rxb1_c%1000))
	{
		p_inpackt_count++;

		#if 0 
		// not-print here
		printf("[%d]:%d rxb=1 show_count= %d\r\n", p_inpackt_count, p_idle_count, show_rxb1_c);
		#endif
				
		return 1;
	}
	return 0;
}

int ethernetif_printdebug_count_rxbyte_0x00(void) /* idle count */
{
	show_rxb0_c++;
			
	if (!(show_rxb0_c%500000))
	{
				p_idle_count++;

		#if 0
		// not-print here
		printf("%d:(%d) rxb=0 show_count= %d\r\n", p_inpackt_count, p_idle_count, show_rxb0_c);
		#endif
				
				return 1;
	}				
	return 0;
}

void ethernetif_printdebug_init(void)
{
	show_Itask_c = 0;
}
void ethernetif_printdebug_input_task(void)
{
	show_Itask_c++;
	if (!(show_Itask_c%3385013)) { //10000, '3385013', 10000000

		printf("Itask, show_count= %d\r\n", show_Itask_c);
	
		#if 1 
		// do-print here
		printf("-- [%d]:%d rxb=1 show_count= %d\r\n", p_inpackt_count, p_idle_count, 
			show_rxb1_c);
		#endif
		#if 1 
		// do-print here
		printf("-- %d:(%d) rxb=0 show_count= %d\r\n", p_inpackt_count, p_idle_count,
			show_rxb0_c);
		#endif
	}
}
