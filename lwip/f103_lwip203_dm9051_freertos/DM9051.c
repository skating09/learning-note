/*********************************************************************************************************//**
* @file    DM9051.c
* @version V1.06
* @org     2014/6/10
* @date    2018/8/29
* @brief   The DM9051 driver for uCOSII+LwIP & uIP on STM32F103.
*************************************************************************************************************
*
* <h2><center>Copyright (C) 2018 DAVICOM Semiconductor Inc. All rights reserved</center></h2>
*
***********************************************************************************************************
* History:
*	V1.02:  2014/8/8
*					1. Add bus driving to 8mA(DM9051_PBCR). For SPI clock upgrade to 36Mhz.
*	V1.03:  2018/8/14
*					2. Add check ISR_PRS bit in DM9051_RX.
*	V1.06:  2018/8/29
*                   3. SPI/STM32F10x/spi_control.c, [PC0 or PA4] selection support
*                   4. Add packet debug show (by define DM9051_SHW option)
*                   5. Add DM9051_Net_Link(void)
*	V1.06.1:2018/8/30
*                   6. Add 'DM9051_SHW_FOR_DHCP' for analysis DHCP stablilty
*                      When after DHCP TX, the RX is allow to have once showing time.
*                      This is to distinguish the fault for DHCP or DM9051_RX. 
*
*	V1.06.2:2018/9/26
*                   7. Support DM9051_Net_RXReset(void) function that the application
*                      can get the RXReset phenomenon's statistic.
*	V1.07: 2018/11/13 
*                   8. Support interrupt mode. Add "int_control.c" integrated in this driver version.
*/

/* Includes ------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include "System_Init.h"
#include "DM9051.h"

#ifdef LwIP_EN
#include "lwip/sys.h"
#include "lwip/inet.h" // jj_add for inet_ntoa()
#include "netconf.h" // xnetif (IP_ADDR0,IP_ADDR1,IP_ADDR2,IP_ADDR3,..)
#include "ethernetif.h"
#else
#include "uip.h"
#include "uip-conf.h"
#endif	//LwIP_EN

#if 1
//[if 0 to check out the DM9051.c DRIVER support functions]
static int statisic_explicity_rxreset_proc = 0;
 
#if DM9051_SHW
#define DM9051_SHW_FOR_DHCP	1 //Select ether for DHCP show, or for generic show
#define MAX_MEM_STR 5
#define MEM_MAGIC_COOKIE	0
#define MEM_OPTION_MSG		1
#define MEM_OPTION_MSG_RX	2
#define MEM_ONCE_PACKET_RX  3
char * const mem_str[MAX_MEM_STR] = {
	"TX_Status [option magic cookie]:", // \r\n
	"TX_Status [option (0) dhcp message type]:", // \r\n
	"RX_Status [option (0) dhcp message type]:", // \r\n
	"RX_Status [(in)-Once-Shwing-Time]:", // \r\n
};
static int allowrx_shw_once = 0;
#define DHCP_MEM_SHW 43
#define MAX_MEM_SHW 336
void mem_shw(char *hdstr, uint8_t *payload, u16_t len)
	{
		u16_t i;
		
		if ((unsigned int)hdstr<MAX_MEM_STR){
			DM9051_TRACE(mem_str[(int)hdstr]);
		} else {
		  DM9051_TRACE(hdstr);
		  DM9051_TRACE("Len [%d]: ", len);
		}
		
		for (i=0; i<len && i<MAX_MEM_SHW; i++)
		{
			if (!(i%16)) DM9051_TRACE("\r\n%04x", i);
			if (!(i%8)) DM9051_TRACE("  ");
			DM9051_TRACE(" %02x", payload[i]);
		}
		DM9051_TRACE("\r\n");
	}
	
#define SIZE_ETH_HDR (14)
#define SIZE_IP_HDR (20)
#define DHCP_CLIENT_PORTNUM  68
#define DHCP_SERVER_PORTNUM  67
	// src port: 68 dst port: 67 	
static int mem_dhcp_tx(uint8_t *buf) // (unsigned char) uint8_t
	{
		#if DM9051_SHW_FOR_DHCP
		int offsetsrc = (SIZE_ETH_HDR + SIZE_IP_HDR);
		int offsetdst = (SIZE_ETH_HDR + SIZE_IP_HDR + 2);
		u16_t src = (buf[offsetsrc] <<8) + buf[offsetsrc+1];
		u16_t dest = (buf[offsetdst] <<8) + buf[offsetdst+1];
		return (src==DHCP_CLIENT_PORTNUM) && (dest==DHCP_SERVER_PORTNUM);
		#else
		return 0;
		#endif
	}
// src port: 67 dst port: 68 	
static int mem_dhcp_rx(uint8_t *buf) // (unsigned char) uint8_t
	{
		#if DM9051_SHW_FOR_DHCP
		int offsetsrc = (SIZE_ETH_HDR + SIZE_IP_HDR);
		int offsetdst = (SIZE_ETH_HDR + SIZE_IP_HDR + 2);
		u16_t src = (buf[offsetsrc] <<8) + buf[offsetsrc+1];
		u16_t dest = (buf[offsetdst] <<8) + buf[offsetdst+1];
		return (src==DHCP_SERVER_PORTNUM) && (dest==DHCP_CLIENT_PORTNUM);
		#else
		return 0;
		#endif
	}
#define OFFSET_ETH_TYPE 	(12)
#define OFFSET_ARP_DSTIP	(38)
// ARP Type: 08 06 	
static int mem_arp_tx(uint8_t *buf)
	{
			return (buf[OFFSET_ETH_TYPE] == 0x08 
				&& buf[OFFSET_ETH_TYPE+1] == 0x06);
	}
// ARP Type: 08 06 	
static int mem_arp_rx(uint8_t *buf)
	{
			return (buf[OFFSET_ETH_TYPE] == 0x08 
				&& buf[OFFSET_ETH_TYPE+1] == 0x06
	#ifdef LwIP_EN
				//ip_addr.addr
				//.&& buf[OFFSET_ARP_DSTIP] == ip4_addr1(&xnetif.ip_addr)  //IP_ADDR0,ip4_addr1_16(netif_ip4_addr(xnetif))
				//.&& buf[OFFSET_ARP_DSTIP+1] == ip4_addr2(&xnetif.ip_addr)  //IP_ADDR1,ip4_addr1_16(netif_ip4_addr(xnetif))
				//.&& buf[OFFSET_ARP_DSTIP+2] == ip4_addr3(&xnetif.ip_addr)  //IP_ADDR2,ip4_addr1_16(netif_ip4_addr(xnetif))
				//.&& buf[OFFSET_ARP_DSTIP+3] == ip4_addr4(&xnetif.ip_addr)  //IP_ADDR3,ip4_addr1_16(netif_ip4_addr(xnetif))
	#endif
				);
	}
	
void DM9051_TXDUMP(uint8_t *Buf, uint16_t SendLength)
{
		static int allowtx_shw = 2;
		#if DM9051_SHW_FOR_DHCP
		allowtx_shw = 0;
		#endif
		if ((allowtx_shw && mem_arp_tx(Buf))
			|| mem_dhcp_tx(Buf))
			//
			// in this case
			// Buf[28]
			// Buf[29]
			// Buf[30]
			// Buf[31]
			// must be equal to ip4_addr1(&xnetif.ip_addr), ...
			//
		{
				if (allowtx_shw && mem_arp_tx(Buf))
					allowtx_shw--;
				if (mem_dhcp_tx(Buf))
					SendLength = DHCP_MEM_SHW;
				mem_shw("TX_Status [out]: \r\n", Buf, SendLength); // JJ dbg
				
				if (mem_dhcp_tx(Buf))
				{
					struct dhcp_msg_frame *mf = (struct dhcp_msg_frame *) (Buf + 14 + 20 + 8);
					mem_shw((void *)MEM_MAGIC_COOKIE, &mf->options[0], 7); // JJ dbg
					mem_shw((void *)MEM_OPTION_MSG, &mf->options[4], 3); // JJ dbg
					
					allowrx_shw_once = 1;
				}
		}
}

void DM9051_RXDUMP(uint8_t *Buf, uint16_t rxlen)
{
		static int allowrx_shw = 2;
		#if DM9051_SHW_FOR_DHCP
		allowrx_shw = 0;
		#endif
	  if ((allowrx_shw && mem_arp_rx(Buf))
		 || mem_dhcp_rx(Buf))
	  {
				if (allowrx_shw && mem_arp_rx(Buf))
					allowrx_shw--;
			
				if (mem_dhcp_rx(Buf))
					rxlen = DHCP_MEM_SHW;

				mem_shw("RX_Status [in]: \r\n", Buf, rxlen); // JJ dbg
				
				if (mem_dhcp_rx(Buf))
				{	
					struct dhcp_msg_frame *mf = (struct dhcp_msg_frame *) (Buf + 14 + 20 + 8);
					mem_shw((void *)MEM_OPTION_MSG_RX, &mf->options[4], 3); // JJ dbg
				}
				
				allowrx_shw_once = 0;
	  }
		else if (allowrx_shw_once)
		{
				mem_shw((void *)MEM_ONCE_PACKET_RX, Buf, rxlen); // JJ dbg
			
				allowrx_shw_once = 0;
		}
}
#endif //DM9051_SHW

/************************************************************************************************************
* @brief  uS level delay function.
* @param  uDelay: Delay time
* @retval None
***********************************************************************************************************/
static void _DM9051_Delay(u32 uDelay)
{
	while (uDelay--);
}

/************************************************************************************************************
* @brief  mS lebel delay function.
* @param  uDelay: delay time based on ms.
* @retval None
***********************************************************************************************************/
static void _DM9051_Delay_ms(u32 uDelay)
{
	u32 i, j;
	
	for (i = 0; i < uDelay; i++ ){
		for (j = 0; j < 1141; j++);
	}
}

/************************************************************************************************************
* @brief  Read function of PHY.
* @param  uReg: PHY register
* @retval uData: Data of register
***********************************************************************************************************/
static u16 phy_read(u32 uReg)
{
	u16 uData;

	/* Fill the phyxcer register into REG_0C */
	DM9051_Write_Reg(DM9051_EPAR, DM9051_PHY | uReg);
	/* Issue phyxcer read command */
	DM9051_Write_Reg(DM9051_EPCR, 0xc);           

	/* Wait read complete */
	//_DM9051_Delay_ms(100);                        
	while(DM9051_Read_Reg(DM9051_EPCR) & 0x1) {_DM9051_Delay(1);}; //Wait complete

	/* Clear phyxcer read command */
	DM9051_Write_Reg(DM9051_EPCR, 0x0); 
	uData = (DM9051_Read_Reg(DM9051_EPDRH) << 8) | DM9051_Read_Reg(DM9051_EPDRL);

	return uData;
}

/*******************************************************************************
* Function Name  : phy_write
* Description    : Write a word to phyxcer
* Input          : - reg: reg
*                  - value: data
* Output         : None
* Return         : None
* Attention         : None
*******************************************************************************/
static void phy_write(uint16_t reg, uint16_t value)
{
	/* Fill the phyxcer register into REG_0C                                                                */
	DM9051_Write_Reg(DM9051_EPAR, DM9051_PHY | reg);

	/* Fill the written data into REG_0D & REG_0E */
	DM9051_Write_Reg(DM9051_EPDRL, (value & 0xff));
	DM9051_Write_Reg(DM9051_EPDRH, ((value >> 8) & 0xff));
	/* Issue phyxcer write command */
	DM9051_Write_Reg(DM9051_EPCR, 0xa);    

	/* Wait write complete */
	//_DM9051_Delay_ms(500);                       
	while(DM9051_Read_Reg(DM9051_EPCR) & 0x1){_DM9051_Delay(1);}; //Wait complete
	
	/* Clear phyxcer write command */
	DM9051_Write_Reg(DM9051_EPCR, 0x0);    
}

/*********************************************************************************************************//**
* @brief  Set PHY mode.
* @param  uMediaMode:
*         @DM9051_AUTO Auto negotiation
*         @DM9051_10MHD 10MHz, Half duplex
*         @DM9051_10MFD 10MHz, Full duplex
*         @DM9051_100MHD 100MHz, Half duplex
*         @DM9051_100MFD 100MHz, Full duplex
* @retval None
***********************************************************************************************************/
static void phy_mode_set(u32 uMediaMode)
{
	uint16_t phy_reg4 = 0x01e1, phy_reg0 = 0x1000;

	if (!(uMediaMode & DM9051_AUTO) )
	{
		switch (uMediaMode)
		{
		case DM9051_10MHD:
			{
				phy_reg4 = 0x21;
				phy_reg0 = 0x0000;
				break;
			}
		case DM9051_10MFD:
			{
				phy_reg4 = 0x41;
				phy_reg0 = 0x1100;
				break;
			}
		case DM9051_100MHD:
			{
				phy_reg4 = 0x81;
				phy_reg0 = 0x2000;
				break;
			}
		case DM9051_100MFD:
			{
				phy_reg4 = 0x101;
				phy_reg0 = 0x3100;
				break;
			}
		case DM9051_10M:
			{
				phy_reg4 = 0x61;
				phy_reg0 = 0x1200;
				break;
			}
		}
		
		/* Set PHY media mode */
		phy_write(4, phy_reg4);
		/* Write rphy_reg0 to Tmp */
		phy_write(0, phy_reg0);
		_DM9051_Delay(10);
	}
#if 0
	DM9051_Write_Reg(DM9051_GPCR, 0x01);  /* Let GPIO0 output                                                 */
	DM9051_Write_Reg(DM9051_GPR,  0x00);  /* Enable PHY                                                       */
#endif	//0
}


#ifndef LwIP_EN
/**************************************************/
/*uIP                                             */
/**************************************************/
void etherdev_init(void)
{
	DM9051_Init();
}

void etherdev_send(void)
{  
	DM9051_TX();	
}

uint16_t etherdev_read(void)
{	
	return DM9051_RX();
}
#endif	//LwIP_EN

#ifdef DM9051_INT_OLD

 ..kbjkhbjhbvhjhjb...

/*********************************************************************************************************//**
* @brief   This function handles EXTI interrupt.
* @retval  None
************************************************************************************************************/
void DM9051_IRQHandler(void)
{
	unsigned  int cpu_sr;
	OS_ENTER_CRITICAL();                         /* Tell uC/OS-II that we are starting an ISR                 */
	OSIntNesting++;
	OS_EXIT_CRITICAL();

#if 0
	if (OSIntNesting > 0)
	{
		OSIntNesting--;
	}
#endif	//0

	EXTI_ClearITPendingBit(EXTI_Line7);

	while( GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_7)==0 )
	{
		DM9051_isr();
	}
	//EXTI_ClearITPendingBit(EXTI_Line7);
	
	/* Tell uC/OS-II that we are leaving the ISR */
	OSIntExit();
}


/*********************************************************************************************************//**
* @brief  Interrupt service routine.
* @retval None
***********************************************************************************************************/

 ..kbjkhbjhbvhjhjb...

void DM9051_ISR(void)
{
	uint16_t int_status;
	u8 temp;

	DM9051_Write_Reg(DM9051_IMR, DM9051_IMR_OFF);		//Clear interrupt flag

	/* Got DM9051 interrupt status  */
	int_status = DM9051_Read_Reg(DM9051_ISR);    

	DM9051_Write_Reg(DM9051_ISR, (u8)int_status);    /* Clear ISR status                                          */
	
	//DM9051_TRACE("DM9051 isr: int status %04x \r\n", int_status); 

	/* receive overflow                                                                                       */
	if (int_status & ISR_ROS)
	{
		DM9051_TRACE2("overflow \r\n");
	}

	if (int_status & ISR_ROOS)
	{
		DM9051_TRACE2("overflow counter overflow \r\n");
	}

	/* Received the coming packet                                                                             */
	if (int_status & ISR_PRS)
	{
		extern OS_EVENT* ethernetinput;
		
		/* disable receive interrupt                                                                            */
		DM9051_device.imr_all &= ~IMR_PRM;

		//DM9051_device.imr_all = IMR_OFF;		//Disable all interrupt.
		
		/* a frame has been received                                                                            */
		OSSemPost(ethernetinput);
	}
#if 0
	/* Transmit Interrupt check                                                                               */
	if (int_status & ISR_PTS)
	{
		/* transmit done                                                                                        */
		int tx_status = DM9051_Read_Reg(DM9051_NSR);    /* Got TX status                                        */

		if (tx_status & (NSR_TX2END | NSR_TX1END))
		{
#if 0    	
			DM9051_device.packet_cnt --;
			if (DM9051_device.packet_cnt > 0)
			{
				DM9051_TRACE("DM9051 isr: tx second packet \r\n");

				/* transmit packet II                                                                               */
				/* Set TX length to DM9051                                                                          */
				DM9051_Write_Reg(DM9051_TXPLL, DM9051_device.queue_packet_len & 0xff);
				DM9051_Write_Reg(DM9051_TXPLH, (DM9051_device.queue_packet_len >> 8) & 0xff);

				/* Issue TX polling command                                                                         */
				DM9051_Write_Reg(DM9051_TCR, TCR_TXREQ);    /* Cleared after TX complete                            */
			}
#endif	//0
			/* unlock DM9051 device                                                                               */
			//OSSemPost(DM9051_sem_ack);	//For TX interrupt
			
		}
	}
#endif	//0

	//DM9051_TRACE("DM9051 isr: imr_all %02x \r\n", DM9051_device.imr_all);
	//DM9051_Write_Reg(DM9051_IMR, DM9051_device.imr_all);
}
#endif	//DM9051_INT_OLD

/*********************************************************************************************************//**
* @brief  Tx function.
* @param  pbuf: buffer link list
* @retval Always 0
***********************************************************************************************************/
#ifdef LwIP_EN
//u32 DM9051_TX(struct netif *netif, struct pbuf* p)
u32 DM9051_TX(uint8_t *TXBuf, uint16_t TxLen)
#else
u32 DM9051_TX(void)
#endif	//LwIP_EN
{
#ifdef FifoPointCheck
	uint16_t calc_MWR;
#endif // FifoPointCheck
	
	uint16_t SendLength;

#ifdef uCOS_EN
	uint8_t err;
#endif
#ifdef LwIP_EN	
	/* Count TX bytes. */
	//netif->txb_count += p->tot_len;
	//SendLength = p->tot_len;
	SendLength = TxLen;
#else
	SendLength = uip_len;
#endif	//LwIP_EN

#ifdef uCOS_EN
	/* lock DM9051 */
	OSSemPend(DM9051_sem_lock, 0,&err);
#endif //uCOS_EN
	
#if DM9051_SHW
	if (1)
	{
		//[TX_debug_print]
	#ifdef LwIP_EN
		DM9051_TXDUMP(TXBuf, SendLength);
	#else
		DM9051_TXDUMP(uip_buf, SendLength);
	#endif //LwIP_EN
	}
#endif	
	
	/* Disable DM9051a interrupt */
	DM9051_Write_Reg(DM9051_IMR, IMR_PAR);

	while(DM9051_Read_Reg(DM9051_TCR) & DM9051_TCR_SET) {
		_DM9051_Delay(5);
	}

#ifdef FifoPointCheck
	/* Ë®àÁ?‰∏ã‰??ãÂÇ≥?ÅÁ??áÈ?‰Ω?, ?•Êé•?∂Èï∑Â∫¶ÁÇ∫Â•áÊï∏ÔºåÈ??†‰?Â∞çÈ??∂Â?ÁØÄ?? */
	/* ?•ÊòØË∂ÖÈ? 0x0bff ÔºåÂ??Ä?ûÊ≠∏ÁπûÂà∞ 0x0000 Ëµ∑Â?‰ΩçÁΩÆ */
	calc_MWR = (DM9051_Read_Reg(DM9051_MWRH) << 8) + DM9051_Read_Reg(DM9051_MWRL);
	calc_MWR += SendLength;
	//printf("calc_MWR = 0x%X\r\n", calc_MWR);
	//	if(SendLength & 0x01) calc_MWR++;
	if(calc_MWR > 0x0bff) 
		calc_MWR -= 0x0c00;
#endif //FifoPointCheck

	/* Move data to DM9051 TX RAM                                                                             */
	//_DM9051_WriteRegAddr(DM9051_MWCMD);

#ifdef LwIP_EN
	/* q traverses through linked list of pbuf's. This list MUST consist of a single packet ONLY            */
	//q = p;

#if 0   
	printf("Tx Read Pointer H = 0x%02X.\r\n", DM9051_Read_Reg(DM9051_TRPAH));
	printf("Tx Read Pointer L = 0x%02X.\r\n", DM9051_Read_Reg(DM9051_TRPAL));
	printf("DM9051_MWRH H = 0x%02X.\r\n", DM9051_Read_Reg(DM9051_MWRH));
	printf("DM9051_MWRL L = 0x%02X.\r\n", DM9051_Read_Reg(DM9051_MWRL));
#endif  //0

	//Write data to FIFO
	//DM9051_Write_Reg(DM9051_TXPLL, q->tot_len & 0xff);
	//DM9051_Write_Reg(DM9051_TXPLH, (q->tot_len >> 8) & 0xff);
	DM9051_Write_Reg(DM9051_TXPLL, TxLen & 0xff);
	DM9051_Write_Reg(DM9051_TXPLH, (TxLen >> 8) & 0xff);
	//DM9051_Write_Mem((u8_t*)q->payload, q->tot_len);
	DM9051_Write_Mem(TXBuf, SendLength);
#else  
	/* Write data to FIFO */
	DM9051_Write_Reg(DM9051_TXPLL, uip_len & 0xff);
	DM9051_Write_Reg(DM9051_TXPLH, (uip_len >> 8) & 0xff);
	DM9051_Write_Mem((u8_t*)uip_buf, SendLength);
#endif	//LwIP_EN
	
	/* Issue TX polling command */
	DM9051_Write_Reg(DM9051_TCR, TCR_TXREQ); /* Cleared after TX complete */

#if 0
	if (DM9051_device.packet_cnt == 0)
	{
		DM9051_TRACE("DM9051 tx: first packet \r\n");

		DM9051_device.packet_cnt ++;
		/* Set TX length to DM9051                                                                              */
		DM9051_Write_Reg(DM9051_TXPLL, p->tot_len & 0xff);
		DM9051_Write_Reg(DM9051_TXPLH, (p->tot_len >> 8) & 0xff);

		/* Issue TX polling command                                                                             */
		DM9051_Write_Reg(DM9051_TCR, TCR_TXREQ);    /* Cleared after TX complete                                */
	}
	else
	{
		DM9051_TRACE("DM9051 tx: second packet \r\n");
		DM9051_device.packet_cnt ++;
		DM9051_device.queue_packet_len = p->tot_len;
	}
#endif	//0

#if 0
	printf("Tx control Reg = 0x%02X.\r\n", DM9051_Read_Reg(DM9051_TCR));
	printf("Tx Read Pointer H = 0x%02X.\r\n", DM9051_Read_Reg(DM9051_TRPAH));
	printf("Tx Read Pointer L = 0x%02X.\r\n", DM9051_Read_Reg(DM9051_TRPAL));
	printf("DM9051_MWRH H = 0x%02X.\r\n", DM9051_Read_Reg(DM9051_MWRH));
	printf("DM9051_MWRL L = 0x%02X.\r\n", DM9051_Read_Reg(DM9051_MWRL));
	printf("Tx control Reg = 0x%02X.\r\n", DM9051_Read_Reg(DM9051_TCR));
	printf("ISR Reg = 0x%02X.\r\n", DM9051_Read_Reg(DM9051_ISR));
	printf("NSR Reg = 0x%02X.\r\n", DM9051_Read_Reg(DM9051_NSR));
#endif  //0


#ifdef FifoPointCheck	
	if(calc_MWR != ((DM9051_Read_Reg(DM9051_MWRH) << 8) + DM9051_Read_Reg(DM9051_MWRL)))
	{
#ifdef LwIP_EN
		//netif->tx_reset++;
#endif	//LwIP_EN
		
#if 0 //Point_Error_Reset
		/* ?•ÊòØ?áÈ??∫ÈåØÔºåÁ?ÂæÖÊ≠§‰∏ÄÂ∞ÅÂ??ÅÂ? , ‰πãÂ??≤Ë??çÁΩÆ */
		while(ior(DM9051_REG_TCR) & DM9051_TCR_SET) udelay (5);
		dm9k_reset();
		/* unlock DM9051 device                                                                                   */
		OSSemPost(DM9051_sem_lock);
		return;
#endif

		printf("DM9K MWR Error!! calc_MWR = 0x%X, SendLength = 0x%x\r\n", calc_MWR, SendLength);
		printf("MWRH = 0x%X, MWRL = 0x%X\r\n", DM9051_Read_Reg(DM9051_MWRH), DM9051_Read_Reg(DM9051_MWRL));
#if 0
		DM9051_Write_Reg(DM9051_ISR, 0x80);    /* DIsable INT                                               */
		DM9051_init(netif);
		DM9051_Write_Reg(DM9051_ISR, 0x81);    /* Enable INT */
#endif	//0

		/*?•ÊòØ?áÈ??∫ÈåØÔºåÂ??áÈ?ÁßªÂà∞‰∏ã‰??ãÂÇ≥?ÅÂ??ÑÂ??≠‰?ÁΩ? */
		DM9051_Write_Reg(DM9051_MWRH, (calc_MWR >> 8) & 0xff);
		DM9051_Write_Reg(DM9051_MWRL, calc_MWR & 0xff);
	}
#endif //FifoPointCheck

	/* Enable DM9051a interrupt    */
	DM9051_Write_Reg(DM9051_IMR, DM9051_device.imr_all);

#ifdef uCOS_EN
	/* unlock DM9051 device */
	OSSemPost(DM9051_sem_lock);

	/* wait ack */
	//OSSemPend(DM9051_sem_ack, 0,&err);	//For TX interrupt
#endif	//uCOS_EN

	return 0;
}

/*********************************************************************************************************//**
* @brief  Rx function.
* @retval pbuf
***********************************************************************************************************/
/**
  * Disable RX interrupt:
  *  Split from 'DM9051_RX()'
  */
void DM9051_RXI_DIS(void)
{
	uint8_t rxchk;
	
	ethernetif_printdebug_input_task();
	
	/* Disable DM9051a interrupt */
	DM9051_Write_Reg(DM9051_IMR, IMR_PAR);
	
	/* Must rx packet available */
	rxchk = DM9051_Read_Reg(DM9051_ISR); 
	if (!(rxchk & ISR_PRS)) { 		
		/* restore receive interrupt */
		//.DM9051_device.imr_all |= IMR_PRM;
		//.DM9051_Write_Reg(DM9051_IMR, DM9051_device.imr_all);
		//.return false;		
	}
	/* clear the rx interrupt-event */
	DM9051_Write_Reg(DM9051_ISR, rxchk);
}

/**
  * Enable RX interrupt:
  *  Split from 'DM9051_RX()'
  */
void DM9051_RXI_EN(void)
{
	/* clear packet received latch status */
	/* restore receive interrupt */
	DM9051_device.imr_all |= IMR_PRM;
	//DM9051_device.imr_all = IMR_DEFAULT;
	DM9051_Write_Reg(DM9051_IMR, DM9051_device.imr_all);
}

/**
  * USING global statistic with:
  *  'statisic_explicity_rxreset_proc'
  */
/**
  * Read RX byte:
  *  Split from 'DM9051_RX()'
  */
uint8_t DM9051_RXBYTE(void)
{
	//return 0x01;
	uint8_t rxbyte;
	/* Check packet ready or not */
	rxbyte = DM9051_Read_Reg(DM9051_MRCMDX);
	rxbyte = DM9051_Read_Reg(DM9051_MRCMDX);
	
	//printf("DM9051_rx process...1st byte = 0x%x\r\n",rxbyte);
	
	if((rxbyte != 1) && (rxbyte != 0))
	{
#if (DM9051_DEBUG == 1) || (DM9051_MSG == 1)
		if (ethernetif_print_opt_err_rxbyte(rxbyte))
				statisic_explicity_rxreset_proc++;
#endif
		
		/* Reset RX FIFO pointer */
		DM9051_Write_Reg(DM9051_RCR, RCR_DEFAULT);	//RX disable
		DM9051_Write_Reg(DM9051_MPCR, 0x01);		//Reset RX FIFO pointer
		_DM9051_Delay(2);
		DM9051_Write_Reg(DM9051_RCR, (RCR_DEFAULT | RCR_RXEN));		//RX Enable

#ifdef LwIP_EN
		//netif->rx_reset++;
#endif	//LwIP_EN
		return 0;
	}
	
#if (DM9051_DEBUG == 1) || (DM9051_MSG == 1)
	if (rxbyte)
	{
		if (ethernetif_printdebug_count_rxbyte_0x01())
			ethernetif_show_inval_begin();
	}
	else
	{
	if (ethernetif_printdebug_count_rxbyte_0x00())
			ethernetif_show_inval_begin();
	}
#endif
	return rxbyte;
}

/**
  * Read RX packet:
  *  Split from 'DM9051_RX()'
  */
uint16_t DM9051_RXPACKET(uint8_t *RxBuf)
{
#ifdef FifoPointCheck
	uint16_t calc_MRR;
#endif  //FifoPointCheck
	uint16_t rx_len = 0;
	
#ifdef uCOS_EN
	uint8_t err;
#endif
#ifdef LwIP_EN	
	//uint8_t* databyte = 0;
	/* init p pointer */
	//p = NULL;
#endif	//LwIP_EN
	
/*
###		
	// RxByte ok than Receive packet 
	if (DM9051_RXBYTE())
	{
*/
		uint16_t rx_status;
		uint8_t ReceiveData[4];
		
#if (DM9051_DEBUG == 1) || (DM9051_MSG == 1)
		ethernetif_print_opt_recovery();
#endif

#ifdef FifoPointCheck			
		calc_MRR = (DM9051_Read_Reg(DM9051_MRRH) << 8) + DM9051_Read_Reg(DM9051_MRRL);	//Save RX SRAM pointer
#endif	//FifoPointCheck
		
		DM9051_Read_Reg(DM9051_MRCMDX);		//dummy read
		
		DM9051_Read_Mem(ReceiveData, 4);
		
		rx_status = ReceiveData[0] + (ReceiveData[1] << 8);
		rx_len = ReceiveData[2] + (ReceiveData[3] << 8);
		
		//DM9051_TRACE("DM9051 rx: status %04x len %d \r\n", rx_status, rx_len);
#ifdef LwIP_EN
		/* Count received bytes. */
		//netif->rxb_count += rx_len;
		/* allocate buffer */
		//p = pbuf_alloc(PBUF_RAW, rx_len, PBUF_POOL);
		
		//if(p != NULL){
			//DM9051_Read_Mem((u8_t *)p->payload, rx_len);
			if (rx_len > DM9051_PKT_MAX){ // read-error
				rx_len = 0; // read-error (keep no change to rx fifo)
			}else{
				DM9051_Read_Mem(RxBuf, rx_len);
			}
		//}
		//else{
			//DM9051_TRACE2("DM9051 rx: no pbuf \r\n");
			/* no pbuf, discard data from DM9051  */
			//DM9051_Read_Mem(databyte, rx_len);
		//}		
#else
		// uip receive data over UIP_CONF_BUFFER_SIZE, return 0
		if(rx_len > UIP_CONF_BUFFER_SIZE){
			DM9051_Free_Mem(rx_len);
			rx_len = 0;
		}else{
			DM9051_Read_Mem((u8_t *)uip_buf, rx_len);
		}
#endif	//LwIP_EN

#if DM9051_SHW
	if (1)
	{
		//[RX_debug_print]
#ifdef LwIP_EN
		DM9051_RXDUMP(RxBuf, rx_len);
#else
		DM9051_RXDUMP(uip_buf, rx_len);
#endif	//LwIP_EN
	}
#endif

#ifdef FifoPointCheck
		/* ?•ÊòØË∂ÖÈ? 0x3fff ÔºåÂ??Ä?ûÊ≠∏ÁπûÂà∞ 0x0c00 Ëµ∑Â?‰ΩçÁΩÆ */
		calc_MRR += (rx_len + 4);
		if(calc_MRR > 0x3fff) calc_MRR -= 0x3400;
		
		if(calc_MRR != ((DM9051_Read_Reg(DM9051_MRRH) << 8) + DM9051_Read_Reg(DM9051_MRRL)))
		{

			printf("DM9K MRR Error!!\r\n");
			printf("Predicut RX Read pointer = 0x%X, Current pointer = 0x%X\r\n", calc_MRR, ((DM9051_Read_Reg(DM9051_MRRH) << 8) + DM9051_Read_Reg(DM9051_MRRL)));
			
			/*?•ÊòØ?áÈ??∫ÈåØÔºåÂ??áÈ?ÁßªÂà∞‰∏ã‰??ãÂ??ÑÂ??≠‰?ÁΩ? */
			DM9051_Write_Reg(DM9051_MRRH, (calc_MRR >> 8) & 0xff);
			DM9051_Write_Reg(DM9051_MRRL, calc_MRR & 0xff);
		}
#endif //FifoPointCheck

		if ((rx_status & 0xbf00) || (rx_len < 0x40) || (rx_len > DM9051_PKT_MAX) )
		{
			//DM9051_TRACE2("rx error: status %04x, rx_len: %d \r\n", rx_status, rx_len);

			if (rx_status & 0x8000)
			{
				DM9051_TRACE2("rx length error \r\n");
			}
			if (rx_len > DM9051_PKT_MAX)
			{
				DM9051_TRACE2("rx length too big \r\n");
				/* RESET device */
				//_DM9051_WriteReg(DM9051_NCR, NCR_RST);
				//_DM9051_Delay_ms(5);
			}
#ifdef LwIP_EN
			//pbuf_free(p);
			//p = NULL;
#endif	//uIP_NOOS
		}

/*
###		
	}else{
		rx_len = 0; // DM9051 rx: No packets received.
	}
*/	

#ifdef LwIP_EN
	//return p;
	return rx_len;
#else
	return rx_len;
#endif	//LwIP_EN	  
}

/**
  * USING global statistic with:
  *  'statisic_explicity_rxreset_proc'
  */
#ifdef LwIP_EN
uint16_t DM9051_RX(uint8_t *RxBuf)
#else
uint16_t DM9051_RX(void)
#endif	//LwIP_EN
{
	uint16_t Len;

#ifdef uCOS_EN
	/* lock DM9051 */
	OSSemPend(DM9051_sem_lock, 0,&err);
#endif //uCOS_EN

	DM9051_RXI_DIS();
	if (DM9051_RXBYTE())
	{
#ifdef LwIP_EN		
		Len = DM9051_RXPACKET(RxBuf);
#else
		Len = DM9051_RXPACKET(uip_buf);
#endif	//LwIP_EN
	}
	DM9051_RXI_EN();

#ifdef uCOS_EN
	/* unlock DM9051 */
	OSSemPost(DM9051_sem_lock);
#endif //uCOS_EN
	return Len;
}

/*********************************************************************************************************
* @brief  DM9051 init function.
* @retval -1 or 0
**********************************************************************************************************/
#ifdef LwIP_EN
s32 DM9051_Init(struct netif *netif)
#else
s32 DM9051_Init()
#endif //LwIP_EN
{
	int i, oft, lnk;
	uint32_t value;
	uint16_t checkch;
	//uint16_t linkch;
	
	ethernetif_printdebug_init();
	
	/* Configure MCU SPI */
	//SPI_Configuration();
	//Interrupt_Configuration();
	
	//DM9051_Write_Reg(DM9051_PBCR, PBCR_MAXDRIVE);
	DM9051_TRACE2("\r\n");
	DM9051_TRACE2("*********************************\r\n");
	DM9051_TRACE2("*** DM9051 freertos_lwip2.0.3 ***\r\n");
	DM9051_TRACE2("*********************************\r\n");
	
test_probe_board:	
	/* Read DM9051 PID / VID, Check MCU SPI Setting correct */
	value = 0;
	value |= (uint32_t)DM9051_Read_Reg(DM9051_VIDL);
	value |= (uint32_t)DM9051_Read_Reg(DM9051_VIDH) << 8;
	value |= (uint32_t)DM9051_Read_Reg(DM9051_PIDL) << 16;
	value |= (uint32_t)DM9051_Read_Reg(DM9051_PIDH) << 24;
	
	if(value != DM9051_ID){ //0x90510a46
		DM9051_TRACE2("DM9051 id error: 0x%x \r\n", value);
		_DM9051_Delay_ms(250);
				
		value = 0;
		value |= (uint32_t)DM9051_Read_Reg(DM9051_VIDL);
		value |= (uint32_t)DM9051_Read_Reg(DM9051_VIDH) << 8;
		value |= (uint32_t)DM9051_Read_Reg(DM9051_PIDL) << 16;
		value |= (uint32_t)DM9051_Read_Reg(DM9051_PIDH) << 24;
		
		if(value != DM9051_ID){ //0x90510a46
			DM9051_TRACE2("DM9051 id: 0x%x \r\n", value);
			DM9051_TRACE2("DM9051 id: read fail ! \r\n");
		}
		//else {
		//}
	}
	//else{
	//}

	if(value == DM9051_ID) //0x90510a46
	{
		/* Show PIV / VID value */
		DM9051_TRACE2("DM9051 id: 0x%x \r\n", value);
		DM9051_TRACE2("DM9051 id: read OK ! \r\n");
		DM9051_TRACE2("\r\n");
	}
	
	#if 1 //debug chip_id & spi write
		DM9051_TRACE2("*** DM9051 test_write_register ***\r\n");
		DM9051_TRACE2("---        To_be_done          ---\r\n");
	
		DM9051_Write_Reg(0x10, 0x55);
		checkch = DM9051_Read_Reg(0x10);
	
		DM9051_TRACE2("---  wr 0x10 data 0x55 read= 0x%02x ---\r\n", checkch);
	
		DM9051_Write_Reg(0x10, 0xaa);
		checkch = DM9051_Read_Reg(0x10);
	
		DM9051_TRACE2("---  wr 0x10 data 0xaa read= 0x%02x ---\r\n", checkch);
		DM9051_TRACE2("\r\n");

		if ((value != DM9051_ID) || (checkch != 0xaa))
		{
		    //test
		    int i;
		    for (i=0; i<80; i++)
			    _DM9051_Delay_ms(250);
		    goto test_probe_board;
		}
	#endif //1
	
	/* Set DM9051 Network Type */
	DM9051_device.type  = TYPE_DM9051;
	
	/* Set Network Speed, Force 10M mode or Auto Mode */
#ifdef FORCE_10M
	.., nnj  test-compiler cfg,,, kbkjb ..
	/* Setting 10M mode */
	DM9051_device.mode = DM9051_10M;
#else
	/* Setting Auto Mode */
	DM9051_device.mode = DM9051_AUTO;
#endif //FORCE_10M
	
	//DM9051_device.packet_cnt = 0;//
	//DM9051_device.queue_packet_len = 0;

	/* SRAM Tx/Rx pointer automatically return to start address */
	/* Packet Transmitted, Packet Received                      */
	/* Enable DM9051 Interrupt Mask*/
	DM9051_device.imr_all = IMR_PAR | IMR_PRM;
	//DM9051_device.imr_all = IMR_DEFAULT;
	//DM9051_device.imr_all = 0x81;

#ifdef LwIP_EN

// .., nnj  test-compiler cfg,,, kbkjb ..
 
	DM9051_device.dev_addr[0] = netif->hwaddr[0];
  DM9051_device.dev_addr[1] = netif->hwaddr[1];
  DM9051_device.dev_addr[2] = netif->hwaddr[2];
  DM9051_device.dev_addr[3] = netif->hwaddr[3];
  DM9051_device.dev_addr[4] = netif->hwaddr[4];
  DM9051_device.dev_addr[5] = netif->hwaddr[5];
#else
	DM9051_device.dev_addr[0] = emacETHADDR0;
  DM9051_device.dev_addr[1] = emacETHADDR1;
  DM9051_device.dev_addr[2] = emacETHADDR2;
  DM9051_device.dev_addr[3] = emacETHADDR3;
  DM9051_device.dev_addr[4] = emacETHADDR4;
  DM9051_device.dev_addr[5] = emacETHADDR5;
#endif	
	
#ifdef uCOS_EN

 .., nnj  test-compiler cfg,,, kbkjb ..
 
	//DM9051_sem_ack = OSSemCreate(0);	//For TX interrupt
	DM9051_sem_lock = OSSemCreate(1);

#ifdef DMA_INT  
	DM9051_txdma_flag = OSSemCreate(0);
	DM9051_rxdma_flag = OSSemCreate(0);
#endif	//DMA_INT

#endif //uCOS_EN

	/* RESET DM9051 device */
	for(i = 0; i <= 1; i++){
		DM9051_Write_Reg(DM9051_NCR, DM9051_REG_RESET);
		_DM9051_Delay(1); //wait 1 us
		DM9051_Write_Reg(DM9051_NCR, 0); // clear NCR
	}

	/* Set DM9051 SPI Type, This Part no standard, According to MCU SPI, Can reference DM9051 DataSheet and MCU SPI Set*/
	DM9051_Write_Reg(DM9051_GPCR, GPCR_GEP_CNTL);
	DM9051_Write_Reg(DM9051_GPR, 0x00);		//Power on PHY
	
	_DM9051_Delay_ms(100);

	/* Set PHY mode */
	phy_mode_set(DM9051_device.mode);
	//printf("PHY REG_0 = 0x%4x\r\n", phy_read(0));
	//printf("PHY REG_4 = 0x%4x\r\n", phy_read(4));
	
	/* Set mac address */
	for (i = 0, oft = DM9051_PAR; i < 6; i++, oft++){
		DM9051_Write_Reg(oft, DM9051_device.dev_addr[i]);
	}

	/* set multicast address */
	for(i = 0; i < 8; i++){ /* Clear Multicast set */
		/* Set Broadcast */
		DM9051_Write_Reg(DM9051_MAR + i, (7 == i) ? 0x80 : 0x00); 
	}
	
	/* Show MAC Address */
	DM9051_TRACE2("DM9051 MAC: ");
	for (i = 0, oft = DM9051_PAR; i < 6; i++, oft++){
		DM9051_TRACE2("%02X:", DM9051_Read_Reg(oft));
#ifndef LwIP_EN
		uip_ethaddr.addr[i] = DM9051_Read_Reg(oft);
#endif
	}
	DM9051_TRACE2("\r\n");

	/************************************************
	*** Activate DM9051 and Setup DM9051 Registers **
	*************************************************/
	/* Clear DM9051 Set and Disable Wakeup function */
	DM9051_Write_Reg(DM9051_NCR, NCR_DEFAULT);
	
	/* Clear TCR Register set */
	DM9051_Write_Reg(DM9051_TCR, TCR_DEFAULT);
	//DM9051_Write_Reg(DM9051_TCR, 0x20); //Disable underrun retry.
	
	/* Discard long Packet and CRC error Packet */
	DM9051_Write_Reg(DM9051_RCR, RCR_DEFAULT);
	/*  Set 1.15 ms Jam Pattern Timer */
	DM9051_Write_Reg(DM9051_BPTR, BPTR_DEFAULT);
	
	/* Open / Close Flow Control */
	//DM9051_Write_Reg(DM9051_FCTR, FCTR_DEAFULT);
	DM9051_Write_Reg(DM9051_FCTR, 0x3A);
	DM9051_Write_Reg(DM9051_FCR,  FCR_DEFAULT);
	//DM9051_Write_Reg(DM9051_FCR,  0x0); //Disable Flow_Control
	
	/* Set Memory Conttrol RegisterÔºåTX = 3KÔºåRX = 13K */
	DM9051_Write_Reg(DM9051_SMCR, SMCR_DEFAULT);
	/* Set Send one or two command Packet*/
	DM9051_Write_Reg(DM9051_TCR2, DM9051_TCR2_SET);
	
	//DM9051_Write_Reg(DM9051_TCR2, 0x80);
	DM9051_Write_Reg(DM9051_INTR, 0x1);

	/* Clear status */
	DM9051_Write_Reg(DM9051_NSR, NSR_CLR_STATUS);
	DM9051_Write_Reg(DM9051_ISR, ISR_CLR_STATUS);
	
	/*while(1){
		linkch = DM9051_Read_Reg(DM9051_NSR);
		if(linkch == 0x40){
			break;
		}
	}*/
	
	//if(linkch == 0x40){
		/* Judge DM9051 PHY speed mode */
		i= 0;
		if ((DM9051_device.mode == DM9051_AUTO) || (DM9051_device.mode == DM9051_10M))
		{
			u16 PHYVAL, PHYVAL_OLD= 0xffff;
			//printf("DM9051_REG_1fh = 0x%X.\r\n", DM9051_Read_Reg(0x1f));
		/*while (!((PHYVAL= phy_read(1)) & 0x20)) Aut Neg. complete*/
		/*while (!((PHYVAL= phy_read(1)) & 0x04)) Link_status: on*/
			while (!((PHYVAL= phy_read(1)) & 0x04))
			{
				if (PHYVAL != PHYVAL_OLD) {
					DM9051_TRACE2("BMSR.t %04X \r\n", PHYVAL);
					PHYVAL_OLD = PHYVAL;
				}
				/* autonegation complete bit */
				_DM9051_Delay(2);
				
				//add delay
				#if 1
				_DM9051_Delay(250);
				#endif
				
				//printf("PHY_REG00h = 0x%04X, PHY_REG01h = 0x%04X.\r\n", phy_read(0), phy_read(1));
				i++;
				
				if (i == 255){
					DM9051_TRACE2("could not establish link \r\n");
					break;
				}
			}
			DM9051_TRACE2("BMSR.e %04X \r\n", PHYVAL);
		}
		
		/* see what we've got */
		if (i!=255)
		{
			lnk = phy_read(17) >> 12;
			DM9051_TRACE2("operating at ");
			switch (lnk)
			{
			case 1:
				DM9051_TRACE2("10M half duplex ");
				//LCD_ShowString(Line5, LEFT_WIDTH, LEFT_ALIGNMENT, Color_White, FONT_12, "Operation Mode: 10M half duplex");
				break;
			case 2:
				DM9051_TRACE2("10M full duplex ");
				//LCD_ShowString(Line5, LEFT_WIDTH, LEFT_ALIGNMENT, Color_White, FONT_12, "Operation Mode: 10M full duplex");
				break;
			case 4:
				DM9051_TRACE2("100M half duplex ");
				//LCD_ShowString(Line5, LEFT_WIDTH, LEFT_ALIGNMENT, Color_White, FONT_12, "Operation Mode: 100M half duplex");
				break;
			case 8:
				DM9051_TRACE2("100M full duplex ");
				//LCD_ShowString(Line5, LEFT_WIDTH, LEFT_ALIGNMENT, Color_White, FONT_12, "Operation Mode: 100M full duplex");
				break;
			default:
				DM9051_TRACE2("unknown: %d ", lnk);
				break;
			}
			DM9051_TRACE2("mode \r\n");
		}
	//}
	DM9051_TRACE2("\r\n");
		
	DM9051_Write_Reg(DM9051_IMR, DM9051_device.imr_all);    /* Enable TX/RX interrupt mask                    */
	//DM9051_Write_Reg(DM9051_IMR, 0x82);
	DM9051_Write_Reg(DM9051_RCR, (RCR_DEFAULT | RCR_RXEN));  /* Enable RX */
	//DM9051_Write_Reg(DM9051_RCR, 0x03);		//promiscuous mode

	return 0;
}

unsigned char flg_if_OFFlinked_on_power_on = 1; //extra-except-disp
/* Interface Check make more deep & detail check to spi INTERFACE AND Net Link STATUS,
 * Return 0, if interface fail or link-down so not available. 
 * Return 1, if DM9051 is avialable.
**/
u8 DM9051_Interface_Check(void)
{
	// JJ tcp
	int read_in;
	u8 PIDL_val; //= DM9051_Read_Reg(DM9051_PIDL);
	u8 LinkStatus; //= DM9051_Read_Reg(DM9051_NSR);
	static unsigned char inputTask_OFFlinked_bit = 1; //Õ¯œﬂ∞Œ±Í÷æ
	static int statisic_piderror_cnt = 0;
	
	read_in = 0;
  READ_IN:	
	LinkStatus = DM9051_Read_Reg(DM9051_NSR);
	LinkStatus = DM9051_Read_Reg(DM9051_NSR);
	PIDL_val = DM9051_Read_Reg(DM9051_PIDL);
		
		// warn: PIDL wrong-print
		if (PIDL_val != 0x51)
		{
			if(!inputTask_OFFlinked_bit){
			  statisic_piderror_cnt++;
			  if (read_in)
			    printf("\r\nethernetif_stat - [PIDL 0x51! but %02x] read_in %d, error_cnt %d\r\nnsr %02x\r\n", 
					PIDL_val, read_in, statisic_piderror_cnt, LinkStatus);
			}
		}
		
	if((LinkStatus) & 0x40) //if (DM9051_Net_Link())
	{
		if(inputTask_OFFlinked_bit) 
		{
			//if (read_in)
			//{
			//	if (read_in==1)
			//		;
			//	else
					//printf("\r\nethernetif_stat - [netif link_up, ip= %s] nsr= %02x read_in %d\r\nPIDL %02x\r\n\r\n", 
					//	inet_ntoa(s_pxNetIf->ip_addr), LinkStatus, read_in, PIDL_val);
					printf("\r\nethernetif_stat - [netif link_up, ip= %s] nsr= %02x read_in %d\r\nPIDL %02x\r\n\r\n", 
						inet_ntoa(xnetif.ip_addr), LinkStatus, read_in, PIDL_val);
			//}
		}
		inputTask_OFFlinked_bit = 0;
	}
	else
	{
		if((!inputTask_OFFlinked_bit) || flg_if_OFFlinked_on_power_on)
		{
			//if (read_in)
				printf("\r\nethernetif_stat - [netif link_down] nsr= %02x\r\nPIDL %02x\r\n", LinkStatus, PIDL_val);
		}
		inputTask_OFFlinked_bit = 1;
		flg_if_OFFlinked_on_power_on = 0; //extra-except
	}
	
		// twice-wrong-PIDL(warn-print) give-up-return, or twice-link-down give-up return.
		if ((PIDL_val != 0x51) || inputTask_OFFlinked_bit)
		{
			if (read_in >= 2) 
			{
				if (PIDL_val != 0x51)
					printf("\r\nethernetif_stat - [GIVE UP A SEMAPHOR] ... read_in %d ...\r\n", read_in);
				
				//xSemaphoreGive(xRxSemaphore);
				//return NULL;
				return 0;
			}
			sys_msleep(1);
			read_in++; //read_in = 1;
			goto READ_IN;
		}
		
	return 1;
}

u8 DM9051_Net_Link(void)
{
	u8 LinkStatus = DM9051_Read_Reg(DM9051_NSR);
	return (LinkStatus & 0x40);
}

int DM9051_Net_RXReset(void)
{
	return statisic_explicity_rxreset_proc;
}
#endif
