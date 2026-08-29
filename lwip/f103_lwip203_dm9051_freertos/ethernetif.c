/**
 * @file
 * Ethernet Interface Skeleton
 *
 */

/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

/*
 * This file is a skeleton for developing Ethernet network interface
 * drivers for lwIP. Add code to the low_level functions and do a
 * search-and-replace for the word "ethernetif" to replace it with
 * something that better describes your network interface.
 */

#include "lwip/opt.h"
#include "lwip/udp.h" //added
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/sys.h"
#include "netif/etharp.h"
#include "err.h"
#include "ethernetif.h"
#include "lwip/prot/dhcp.h" //added
#include "lwip/timeouts.h" //added
#include "lwip/inet.h" // jj_add for inet_ntoa()

#include "main.h"
//#include "stm32f4x7_eth.h"
#include <string.h>
#include "System_Init.h"
#include "DM9051.h"

#define netifMTU                                (1500)
#define netifINTERFACE_TASK_STACK_SIZE					( 350 )
#define netifINTERFACE_TASK_PRIORITY						( configMAX_PRIORITIES - 1 )
#define netifGUARD_BLOCK_TIME										( 250 )
/* The time to block waiting for input. */
#define emacBLOCK_TIME_WAITING_FOR_INPUT				( ( portTickType ) 200 )

/* Define those to better describe your network interface. */
#define IFNAME0 's'
#define IFNAME1 't'

static struct netif *s_pxNetIf = NULL;
xSemaphoreHandle s_xSemaphore1 = NULL;

#if 0
/* Ethernet buffer  */
__align(4)
uint8_t ETH_Buff[1520];
#define Rx_Buff ETH_Buff
#define Tx_Buff ETH_Buff
//__align(4)
//uint8_t Rx_Buff[1520];
//__align(4)
//uint8_t Tx_Buff[1520];

#else
/* Ethernet Receive buffers  */
__align(4)
uint8_t Rx_Buff[1520]; //uint8_t Rx_Buff[ETH_RXBUFNB][ETH_MAX_PACKET_SIZE]; 

/* Ethernet Transmit buffers */
__align(4)
uint8_t Tx_Buff[1520]; //uint8_t Tx_Buff[ETH_TXBUFNB][ETH_MAX_PACKET_SIZE]; 

#endif

/* Global pointers to track current transmit and receive descriptors */
//static void ethernetif_input( void * pvParameters );
static void arp_timer(void *arg);

// buffer for compose the mem show's header
//.static uint8_t temp_head_show[50]; 
	
// src port: 68 dst port: 67 	
// src port: 67 dst port: 68 	
//static int mem_udp_dhcp_tx(uint8_t *payload) // (unsigned char) uint8_t
//	{
//		struct udp_hdr *udphdr = (struct udp_hdr *)(payload + SIZEOF_ETH_HDR + IP_HLEN);
//		u16_t src = ntohs(udphdr->src);
//		u16_t dest = ntohs(udphdr->dest);
//		return (src==DHCP_CLIENT_PORT) && (dest==DHCP_SERVER_PORT);
//	}
//static int mem_udp_dhcp_rx(uint8_t *payload) // (unsigned char) uint8_t
//	{
//		struct udp_hdr *udphdr = (struct udp_hdr *)(payload + SIZEOF_ETH_HDR + IP_HLEN);
//		u16_t src = ntohs(udphdr->src);
//		u16_t dest = ntohs(udphdr->dest);
//		return (src==DHCP_SERVER_PORT) && (dest==DHCP_CLIENT_PORT);
//	}
	
void low_level_begin_init(struct netif *netif)
{
  /* set netif MAC hardware address length */
  netif->hwaddr_len = ETHARP_HWADDR_LEN;
	
  /* set netif MAC hardware address */
  netif->hwaddr[0] =  emacETHADDR0;
  netif->hwaddr[1] =  emacETHADDR1;
  netif->hwaddr[2] =  emacETHADDR2;
  netif->hwaddr[3] =  emacETHADDR3;
  netif->hwaddr[4] =  emacETHADDR4;
  netif->hwaddr[5] =  emacETHADDR5;
  
  /* set netif maximum transfer unit */
  netif->mtu = 1500;

  /* Accept broadcast address and ARP traffic */
  netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;
  
  s_pxNetIf =netif;
 
	DM9051_Init(s_pxNetIf);
}	
/**
 * In this function, the hardware should be initialized.
 * Called from ethernetif_init().
 *
 * @param netif the already initialized lwip network interface structure
 *        for this ethernetif
 */
static void low_level_init(struct netif *netif)
{
  /* set netif MAC hardware address length */
  netif->hwaddr_len = ETHARP_HWADDR_LEN;
	
  /* set netif MAC hardware address */
  netif->hwaddr[0] =  emacETHADDR0;
  netif->hwaddr[1] =  emacETHADDR1;
  netif->hwaddr[2] =  emacETHADDR2;
  netif->hwaddr[3] =  emacETHADDR3;
  netif->hwaddr[4] =  emacETHADDR4;
  netif->hwaddr[5] =  emacETHADDR5;
  
  /* set netif maximum transfer unit */
  netif->mtu = 1500;

  /* Accept broadcast address and ARP traffic */
  netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;
  
  s_pxNetIf =netif;
 
	DM9051_Init(s_pxNetIf);
	
  /* create binary semaphore used for informing ethernetif of frame reception */
  if (s_xSemaphore1 == NULL){
    s_xSemaphore1= xSemaphoreCreateCounting(20,0);
  }
	
  /* create the task that handles the ETH_MAC */
	xTaskCreate(ethernetif_input, (char*) "Eth_if", netifINTERFACE_TASK_STACK_SIZE, NULL,
						netifINTERFACE_TASK_PRIORITY,NULL);
		#if 0
		// How (why not) use "sys_arch.c"?
		sys_thread_new((char*) "Eth_if", ethernetif_input, NULL, TCPIP_THREAD_STACKSIZE, TCPIP_THREAD_PRIO);
		#endif
}


/**
 * This function should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @param p the MAC packet to send (e.g. IP packet including MAC addresses and type)
 * @return ERR_OK if the packet could be sent
 *         an err_t value if the packet couldn't be sent
 *
 * @note Returning ERR_MEM here if a DMA queue of your MAC is full can lead to
 *       strange results. You might consider waiting for space in the DMA queue
 *       to become availale since the stack doesn't retry to send a packet
 *       dropped because of memory failure (except for the TCP timers).
 */
extern struct netif xnetif;

static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
  static xSemaphoreHandle xTxSemaphore = NULL;
  struct pbuf *q;
  uint32_t l = 0;
  //u8 *buffer ;
  
  if (xTxSemaphore == NULL)
  {
    vSemaphoreCreateBinary (xTxSemaphore);
  } 
   
  if (xSemaphoreTake(xTxSemaphore, netifGUARD_BLOCK_TIME))
  {
	int w = 0, wl = 0;
    int x = 0; // added
	int wc = 0;
    for(q = p; q != NULL; q = q->next) {
			wl += q->len;
			w++; 
			x++;
	}
	if (w==1 && wl>=1024)
	{
			w= x= 2;
	}
//.	int s = 0; // added
    for(q = p; q != NULL; q = q->next) 
    {
//.		if (mem_udp_dhcp_tx(q->payload)) {
//.			 snprintf(temp_head_show, sizeof(temp_head_show), "Semaphore q%d, TX [dhcp.out]: \r\n", s++); // added 
//.			 mem_shw(temp_head_show, q->payload, DHCP_MEM_SHW /*q->len*/); // JJ dbg
//.		}
		memcpy(&Tx_Buff[l], q->payload, q->len);
		l = l + q->len;
		if (w>1) printf("-------- %d. TX q->len = %d\r\n", w-x, q->len);
		x--;
			
		wc++;
    }
	if (w>1) printf("-------- %d. DM9051_TX (chip tx acc total len = %d)\r\n", wc, l);
	
	//.printf("-again.e3 xnetif ip4_addr_netcmp: %s\r\n", 		// [My IP = Web server IP]
	//.  inet_ntoa(xnetif.ip_addr));
	//.printf("-again.e3 xnetif ip4_addr_netcmp: %s\r\n", // [netmask IP]
	//.  inet_ntoa(xnetif.netmask));
	
	DM9051_TX(Tx_Buff, l);
	
	//.printf("-again.e8 xnetif ip4_addr_netcmp: %s\r\n", 		// [My IP = Web server IP]
	//.  inet_ntoa(xnetif.ip_addr));
	//.printf("-again.e8 xnetif ip4_addr_netcmp: %s\r\n", // [netmask IP]
	//.  inet_ntoa(xnetif.netmask));
	
    xSemaphoreGive(xTxSemaphore);
  }
  return ERR_OK;
}

#if 0
/* Called by app who want to 'send' with-using low_level_input(). JJ tcp */
/* Called by app who want to 'send' with-using low_level_output(). ~xx~not */
/* E.g.
 *    app: tcp_test.c
 *
 *		while (ethernetif_output_sem() != pdTRUE) ;             <--- first. in advance.
 *    sent_data = http_write(socket_fd, tcp_send, 1024 * 3);  <--- then, call to continous do send several packets.
 *
 */
long ethernetif_output_sem( void )
{
	if (s_xSemaphore)
	{
		return xSemaphoreTake( s_xSemaphore, emacBLOCK_TIME_WAITING_FOR_INPUT);
	}
	return pdFALSE;
}
#endif


/**
 * Acc-counter
 */
//static int show_rxb1_c = 0, p_inpackt_count= 0;
//static int show_rxb0_c = 0, p_idle_count= 0;
//int ethernetif_printdebug_count_rxbyte_0x01(void) /* valid pkt count */
//{
//	show_rxb1_c++;
//			
//	if (!(show_rxb1_c%1000))
//	{
//		p_inpackt_count++;

		#if 0 
		// not-print here
		printf("[%d]:%d rxb=1 show_count= %d\r\n", p_inpackt_count, p_idle_count, show_rxb1_c);
		#endif
				
//		return 1;
//	}
//	return 0;
//}
//int ethernetif_printdebug_count_rxbyte_0x00(void) /* idle count */
//{
//	show_rxb0_c++;
			
//	if (!(show_rxb0_c%500000))
//	{
//				p_idle_count++;

		#if 0
		// not-print here
		printf("%d:(%d) rxb=0 show_count= %d\r\n", p_inpackt_count, p_idle_count, show_rxb0_c);
		#endif
				
//				return 1;
//	}				
//	return 0;
//}

//static int show_inval_c = 0; //=ethernetif_show_inval_begin(void)
//void ethernetif_show_inval_begin(void)
//{
//	show_inval_c = 0; // begin to show 'n' times inval
//}
//void ethernetif_print_opt_recovery(void)
//{
//		if (show_inval_c)
//		{
//			show_inval_c = 0;
//			printf("*************************************\r\n");
//			printf("*** DM9051 inval show_count.clear (RX recovery-OK) ***\r\n");
//			printf("*************************************\r\n");
//		}
//}
//int ethernetif_print_opt_err_rxbyte(uint8_t rxbyte)
//{
//	int ret = 0;
//	if (show_inval_c < 3){
//		printf("\r\n");
//		if (show_inval_c==0) {
//			printf("*******************************************\r\n");
//			printf("*** DM9051 inval show_count.error.found (Explicity, Reset the RX FIFO pointer ) ***\r\n");
//			printf("*******************************************\r\n");
//			ret = 1; //statisic_explicity_rxreset_proc++;
//		}
		/* Check to continue working ability ... */
//		printf("inval show_count= %d\r\n", show_inval_c);
//		printf("rxbyte = %02X.\r\n", rxbyte);
//		printf("DM9051 rx: rx error, stop device \r\n");
//		show_inval_c++;
//	}
//	return ret;
//}

/**
 * Should allocate a pbuf and transfer the bytes of the incoming
 * packet from the interface into the pbuf.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return a pbuf filled with the received packet (including MAC header)
 *         NULL on memory error
 */
//int low_level_input_status_check(void)
//{
//.	return DM9051_Interface_Check();
//}
//void low_level_input_rxbyte_check(void) // hint: low_level_interrupt_rxbyte_check()
//{
//.	DM9051_RXI_DIS();
//}
//void low_level_input_rxbyte_check_end(void)
//{
//.	DM9051_RXI_EN();
//}

static struct pbuf * low_level_input(struct netif *netif)
{
  struct pbuf *p, *q;
  u16_t len;
  int l =0;
  static xSemaphoreHandle xRxSemaphore = NULL;
  //u8 *buffer;

	p = NULL;	
	
	if (xRxSemaphore == NULL)
	{
		vSemaphoreCreateBinary (xRxSemaphore);
 	}
	
	if (xSemaphoreTake(xRxSemaphore, netifGUARD_BLOCK_TIME))
	{
		/* Be operated in vTask4() / main.c */
		//if (!DM9051_Interface_Check())
		//{
		//	xSemaphoreGive(xRxSemaphore);
		//	return NULL;
		//}
#if 0
// JJ tcp
#endif
		if ((len = DM9051_RXPACKET(Rx_Buff))) // TT tcp.clarify ('DM9051_RXPACKET' instead of 'DM9051_RX')
		{
				/* We allocate a pbuf chain of pbufs from the pool. */
				p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);

				if (p != NULL)
				{
		//.			int s = 0; // added
					for (q = p; q != NULL; q = q->next)
					{
						memcpy((u8_t*)q->payload, &Rx_Buff[l], q->len);
						l = l + q->len;
						//printf("-------- RX q->len = %d\r\n", q->len);
						
		//.				if (q->len && mem_udp_dhcp_rx(q->payload))
		//.				{
		//.				  snprintf(temp_head_show, sizeof(temp_head_show), "Semaphore q%d, RX [dhcp.in]: \r\n", s++); // added 
		//.				  mem_shw(temp_head_show, q->payload, DHCP_MEM_SHW /*q->len*/); // JJ dbg
		//.				}
					}
					//printf("-------- low_level_input (acc rx total len = %d) (chip RX %d)\r\n", l, len);			
				}
		}
		xSemaphoreGive(xRxSemaphore);
	}
  return p;
}


/**
 * This function is the ethernetif_input task, it is processed when a packet 
 * is ready to be read from the interface. It uses the function low_level_input() 
 * that should handle the actual reception of bytes from the network
 * interface. Then the type of the received packet is determined and
 * the appropriate input function is called.
 *
 * @param netif the lwip network interface structure for this ethernetif
 */
void ethernetif_input( void * pvParameters )
{
  struct pbuf *p;
  
  for( ;; )
  {
    if (xSemaphoreTake( s_xSemaphore1, emacBLOCK_TIME_WAITING_FOR_INPUT)==pdTRUE)
    {
		do {
			p = low_level_input( s_pxNetIf );
			if (p != NULL) { //JJ tcp - ADD //..f, ., fbdf
				
				#if 1
				// One more layer.s -- skip no data len pbuf
				//. if (p->len==0)
				//. {
				//. 	pbuf_free(p);
				//. 	p=NULL;
				//. }
				//. else
				// One more layer.e -- skip no data len pbuf
				#endif
				
				if (ERR_OK != s_pxNetIf->input( p, s_pxNetIf))
				{
					pbuf_free(p);
					p=NULL;
				}
			}
		} while (DM9051_RXBYTE()); // multi-rx-
		DM9051_RXI_EN();
    }
	//xSemaphoreGive( s_xSemaphore );
  }
}  
    
void ethernetif_begining_init(struct netif *netif)
{
#if LWIP_NETIF_HOSTNAME
  /* Initialize interface hostname */
  netif->hostname = "lwip";
#endif /* LWIP_NETIF_HOSTNAME */

  netif->name[0] = IFNAME0;
  netif->name[1] = IFNAME1;

  netif->output = etharp_output;
  netif->linkoutput = low_level_output;
}

void ethernetif_ending_arptimer(void)
{
	etharp_init(); /* Compatibility define, no init needed. */
  sys_timeout(ARP_TMR_INTERVAL, arp_timer, NULL);
}

/**
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 * This function should be passed as a parameter to netif_add().
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return ERR_OK if the loopif is initialized
 *         ERR_MEM if private data couldn't be allocated
 *         any other err_t on error
 */
err_t ethernetif_init(struct netif *netif)
{
  LWIP_ASSERT("netif != NULL", (netif != NULL));

#if LWIP_NETIF_HOSTNAME
  /* Initialize interface hostname */
  netif->hostname = "lwip";
#endif /* LWIP_NETIF_HOSTNAME */

  netif->name[0] = IFNAME0;
  netif->name[1] = IFNAME1;

  netif->output = etharp_output;
  netif->linkoutput = low_level_output;

  /* initialize the hardware */
  low_level_init(netif);
  
  etharp_init();
  sys_timeout(ARP_TMR_INTERVAL, arp_timer, NULL);
  return ERR_OK;
}

static void arp_timer(void *arg)
{
  etharp_tmr();
  sys_timeout(ARP_TMR_INTERVAL, arp_timer, NULL);
}
