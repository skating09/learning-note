/*********************************************************************************************************//**
* @file    DM9051.h
* @version V1.06
* @org     2014/6/10
* @date    2018/8/29
* @brief   The header file of DM9051 register.
*************************************************************************************************************
*
* <h2><center>Copyright (C) 2018 DAVICOM Semiconductor Inc. All rights reserved</center></h2>
*
************************************************************************************************************/
/* Define to prevent recursive inclusion -------------------------------------------------------------------*/
#ifndef __DM9051_REG_H
#define __DM9051_REG_H

/* Settings ------------------------------------------------------------------------------------------------*/
#define DM9051_MSG 		  	1 /* Show DM9051 Message */
#define DM9051_DEBUG      1 /* Show DM9051 Debug Message */
#define DM9051_SHW      1 /* Show DM9051 Packet Messages */

#define LwIP_EN             /* Use LwIP, This define mask use uIP*/
//#define DMA_INT						/* SPI DMA interrupt */
//#define SPI_DMA							/* DMA transfer SPI data */
//#define DM9051_INT_OLD	    	/* Packet RX interrupt machine */
#define DM9051_INT
//#define FifoPointCheck
//#define FORCE_10M         /* This define mask use Auto mode, else use force 10M mode */
//#define uCOS_EN           /* If use uC/OS-II unmask */

/* Includes ------------------------------------------------------------------------------------------------*/
#ifdef LwIP_EN
#include "stdint.h"
#else
  /* Includes-------------------------------------------------------------------------------------------------*/
  /* For STM project: #include "include.h" is required. */
  #if 0
  #include "includes.h"
  #endif
  /* ---------------------------------------------------------------------------------------------------------*/
#endif	//LwIP_EN

#ifdef LwIP_EN
#include "ethernetif.h"
#endif	//LwIP_EN

#define emacETHADDR0   0x00
#define emacETHADDR1   0x60
#define emacETHADDR2   0x6e
#define emacETHADDR3   0x90 //0x00 //0x90
#define emacETHADDR4   0x51 //0xff //0x51
#define emacETHADDR5   0x02 //0x09 //0x02

typedef int32_t  s32;
typedef int16_t s16;
typedef int8_t  s8;

typedef uint32_t  u32;
typedef uint16_t u16;
typedef uint8_t  u8;

#if (DM9051_DEBUG == 1)
#define DM9051_TRACE  printf
#else
#define DM9051_TRACE(...)
#endif

#if (DM9051_MSG == 1)
#define DM9051_TRACE2  printf
#else
#define DM9051_TRACE2(...)
#endif

/* Private typedef -----------------------------------------------------------------------------------------*/
enum DM9051_PHY_mode
{
	DM9051_10MHD   = 0,
	DM9051_100MHD  = 1,
	DM9051_10MFD   = 4,
	DM9051_100MFD  = 5,
	DM9051_10M     = 6,
	DM9051_AUTO    = 8,
	DM9051_1M_HPNA = 0x10
};

enum DM9051_TYPE
{
	TYPE_DM9051E,
	TYPE_DM9051A,
	TYPE_DM9051B,
	TYPE_DM9051
};

struct DM9051_eth
{
	enum DM9051_TYPE type;
	enum DM9051_PHY_mode mode;

	uint8_t  imr_all;
	
	/* packet I or II */
	uint8_t  packet_cnt;
	/* queued packet (packet II) */
	uint16_t queue_packet_len;

	/* interface address info. */
	uint8_t  dev_addr[6]; /* hw address */
	
	/* Byte counter */
	//uint32_t_t txb_count;	
	//uint32_t_t rxb_count;  
};
static struct DM9051_eth DM9051_device;

/* Private constants ---------------------------------------------------------------------------------------*/
#define DM9051_PHY          (0x40)    			/* PHY address 0x01                                             */

/* Exported typedef ----------------------------------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------------------------------------*/ 
#define DM9051_ID           (0x90510A46)		/* DM9051A ID                                                   */
#define DM9051_PKT_MAX      (1536)          /* Received packet max size                                     */
#define DM9051_PKT_RDY      (0x01)          /* Packet ready to receive                                      */

#define DM9051_NCR          (0x00)
#define DM9051_NSR          (0x01)
#define DM9051_TCR          (0x02)
#define DM9051_TSR1         (0x03)
#define DM9051_TSR2         (0x04)
#define DM9051_RCR          (0x05)
#define DM9051_RSR          (0x06)
#define DM9051_ROCR         (0x07)
#define DM9051_BPTR         (0x08)
#define DM9051_FCTR         (0x09)
#define DM9051_FCR          (0x0A)
#define DM9051_EPCR         (0x0B)
#define DM9051_EPAR         (0x0C)
#define DM9051_EPDRL        (0x0D)
#define DM9051_EPDRH        (0x0E)
#define DM9051_WCR          (0x0F)

#define DM9051_PAR          (0x10)
#define DM9051_MAR          (0x16)

#define DM9051_GPCR         (0x1e)
#define DM9051_GPR          (0x1f)
#define DM9051_TRPAL        (0x22)
#define DM9051_TRPAH        (0x23)
#define DM9051_RWPAL        (0x24)
#define DM9051_RWPAH        (0x25)

#define DM9051_VIDL         (0x28)
#define DM9051_VIDH         (0x29)
#define DM9051_PIDL         (0x2A)
#define DM9051_PIDH         (0x2B)

#define DM9051_CHIPR        (0x2C)
#define DM9051_TCR2         (0x2D)
#define DM9051_OTCR         (0x2E)
#define DM9051_SMCR         (0x2F)

#define DM9051_ETCR         (0x30)    /* early transmit control/status register                             */
#define DM9051_CSCR         (0x31)    /* check sum control register                                         */
#define DM9051_RCSSR        (0x32)    /* receive check sum status register                                  */

#define DM9051_PBCR					(0x38)
#define DM9051_INTR					(0x39)
#define DM9051_MPCR					(0x55)
#define DM9051_MRCMDX       (0x70)
#define DM9051_MRCMD        (0x72)
#define DM9051_MRRL         (0x74)
#define DM9051_MRRH         (0x75)
#define DM9051_MWCMDX       (0x76)
#define DM9051_MWCMD        (0x78)
#define DM9051_MWRL         (0x7A)
#define DM9051_MWRH         (0x7B)
#define DM9051_TXPLL        (0x7C)
#define DM9051_TXPLH        (0x7D)
#define DM9051_ISR          (0x7E)
#define DM9051_IMR          (0x7F)

#define CHIPR_DM9051A       (0x19)
#define CHIPR_DM9051B       (0x1B)

#define DM9051_REG_RESET    (0x01)
#define DM9051_IMR_OFF      (0x80)
#define DM9051_TCR2_SET     (0x90)	/* set one packet */
#define DM9051_RCR_SET      (0x31)
#define DM9051_BPTR_SET     (0x37)
#define DM9051_FCTR_SET     (0x38)
#define DM9051_FCR_SET      (0x28)
#define DM9051_TCR_SET      (0x01)


#define NCR_EXT_PHY         (1 << 7)
#define NCR_WAKEEN          (1 << 6)
#define NCR_FCOL            (1 << 4)
#define NCR_FDX             (1 << 3)
#define NCR_LBK             (3 << 1)
#define NCR_RST             (1 << 0)
#define NCR_DEFAULT					(0x0)    /* Disable Wakeup */

#define NSR_SPEED           (1 << 7)
#define NSR_LINKST          (1 << 6)
#define NSR_WAKEST          (1 << 5)
#define NSR_TX2END          (1 << 3)
#define NSR_TX1END          (1 << 2)
#define NSR_RXOV            (1 << 1)
#define NSR_CLR_STATUS			(NSR_WAKEST | NSR_TX2END | NSR_TX1END)

#define TCR_TJDIS           (1 << 6)
#define TCR_EXCECM          (1 << 5)
#define TCR_PAD_DIS2        (1 << 4)
#define TCR_CRC_DIS2        (1 << 3)
#define TCR_PAD_DIS1        (1 << 2)
#define TCR_CRC_DIS1        (1 << 1)
#define TCR_TXREQ           (1 << 0) /* Start TX */
#define TCR_DEFAULT					(0x0)

#define TSR_TJTO            (1 << 7)
#define TSR_LC              (1 << 6)
#define TSR_NC              (1 << 5)
#define TSR_LCOL            (1 << 4)
#define TSR_COL             (1 << 3)
#define TSR_EC              (1 << 2)

#define RCR_WTDIS           (1 << 6)
#define RCR_DIS_LONG        (1 << 5)
#define RCR_DIS_CRC         (1 << 4)
#define RCR_ALL             (1 << 3)
#define RCR_RUNT            (1 << 2)
#define RCR_PRMSC           (1 << 1)
#define RCR_RXEN            (1 << 0)
#define RCR_DEFAULT					(RCR_DIS_LONG | RCR_DIS_CRC)

#define RSR_RF              (1 << 7)
#define RSR_MF              (1 << 6)
#define RSR_LCS             (1 << 5)
#define RSR_RWTO            (1 << 4)
#define RSR_PLE             (1 << 3)
#define RSR_AE              (1 << 2)
#define RSR_CE              (1 << 1)
#define RSR_FOE             (1 << 0)

#define BPTR_DEFAULT				(0x3f)
#define FCTR_DEAFULT				(0x38)
#define FCR_DEFAULT					(0xFF)
#define SMCR_DEFAULT				(0x0)
#define PBCR_MAXDRIVE				(0x44)

//#define FCTR_HWOT(ot)       ((ot & 0xF ) << 4 )
//#define FCTR_LWOT(ot)       (ot & 0xF )

#define IMR_PAR             (1 << 7)
#define IMR_LNKCHGI         (1 << 5)
#define IMR_UDRUN						(1 << 4)
#define IMR_ROOM            (1 << 3)
#define IMR_ROM             (1 << 2)
#define IMR_PTM             (1 << 1)
#define IMR_PRM             (1 << 0)
#define IMR_FULL 						(IMR_PAR | IMR_LNKCHGI | IMR_UDRUN | IMR_ROOM | IMR_ROM | IMR_PTM | IMR_PRM)
#define IMR_OFF							(IMR_PAR)
#define IMR_DEFAULT					(IMR_PAR | IMR_PRM | IMR_PTM) 

#define ISR_ROOS            (1 << 3)
#define ISR_ROS             (1 << 2)
#define ISR_PTS             (1 << 1)
#define ISR_PRS             (1 << 0)
#define ISR_CLR_STATUS      (0x80 | 0x3F)

#define EPCR_REEP           (1 << 5)
#define EPCR_WEP            (1 << 4)
#define EPCR_EPOS           (1 << 3)
#define EPCR_ERPRR          (1 << 2)
#define EPCR_ERPRW          (1 << 1)
#define EPCR_ERRE           (1 << 0)

#define GPCR_GEP_CNTL       (1 << 0)

#define SPI_WR_BURST				(0xF8)
#define SPI_RD_BURST				(0x72)

#define SPI_READ						(0x03)
#define SPI_WRITE        		(0x04)
#define SPI_WRITE_BUFFER  	(0x05)		/* Send a series of bytes from the Master to the Slave */
#define SPI_READ_BUFFER   	(0x06)    /* Send a series of bytes from the Slave  to the Master */


/* Exported macro ------------------------------------------------------------------------------------------*/
/* Exported variables --------------------------------------------------------------------------------------*/
/* Exported functions --------------------------------------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------------------------------------*/

/* PHY function prototypes */
static uint16_t phy_read(uint32_t uReg);
static void phy_write(uint16_t uReg, uint16_t uValue);
static void phy_mode_set(uint32_t uMediaMode);

/* Delay function prototypes */
static void _DM9051_Delay(uint32_t uDelay);
static void _DM9051_Delay_ms(uint32_t uDelay);

/* Configure MCU SPI function prototypes */
//static void DM9051_Configuration(void);

void DM9051_RXI_DIS(void);
void DM9051_RXI_EN(void);
uint8_t DM9051_RXBYTE(void);
uint16_t DM9051_RXPACKET(uint8_t *RxBuf);

#ifdef LwIP_EN
s32 DM9051_Init(struct netif *netif);
u32 DM9051_TX(uint8_t *TXBuf, uint16_t TxLen);
uint16_t DM9051_RX(uint8_t *RxBuf);
#else
s32 DM9051_Init(void);
uint32_t DM9051_TX(void);
uint16_t DM9051_RX(void);
void etherdev_init(void);
void etherdev_send(void);
uint16_t etherdev_read(void);
#endif	//LwIP_EN

#ifdef DM9051_INT_OLD
void DM9051_isr(void);
#endif //DM9051_INT_OLD

u8 DM9051_Interface_Check(void);
u8 DM9051_Net_Link(void); // return true, while link-up
int DM9051_Net_RXReset(void); // rx error reset_loop counter

void mem_shw(char *hdstr, uint8_t *payload, u16_t len);

struct dhcp_msg_frame {
  u8_t op, htype, hlen, hops;
  u8_t xid[4];
  u16_t secs, flags;
  u8_t ciaddr[4];
  u8_t yiaddr[4];
  u8_t siaddr[4];
  u8_t giaddr[4];
  u8_t chaddr[16];
#ifndef UIP_CONF_DHCP_LIGHT
  u8_t sname[64];
  u8_t file[128];
#endif
  u8_t options[312];
};

#endif /* __DM9051_REG_H -----------------------------------------------------------------------------------*/
