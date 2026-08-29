#include "stdio.h"
#include "stm32f10x.h"
#include "DM9051.h"

#if 0
/*   SPI Initialization  */
# declare void SPI_Configuration(void) function proto-type in System_Init.h

# call SPI_Configuration(void) function in "System_Initialization(void)" in System_Init.c

# implement void SPI_Configuration(void) function in this file spi_control.c
//void SPI_Configuration(void)
//{
//}
#endif

/**
 *
 */

/* DM9051 */
//------------- CS = [PC0 or PA4] -------------
//(was) //PC0
//#define SELECT_GPIO_CS									GPIOC
//#define SELECT_PIN_CS										GPIO_Pin_0 //(PC0)
//#define SELECT_Periph_CS									RCC_APB2Periph_GPIOC
//(is) //PA4
//#define SELECT_GPIO_CS									GPIOA
//#define SELECT_PIN_CS										GPIO_Pin_4 //(PA4)
//#define SELECT_Periph_CS									RCC_APB2Periph_GPIOA
//(now) .select
#define SELECT_GPIO_CS										GPIOA
#define SELECT_PIN_CS										GPIO_Pin_4
#define SELECT_Periph_CS									RCC_APB2Periph_GPIOA

#define GPIO_DM9051_CS										SELECT_GPIO_CS
#define RCC_APB2Periph_GPIO_DM9051_CS  		                SELECT_Periph_CS
#define GPIO_Pin_DM9051_CS								    SELECT_PIN_CS

#define DM9051_SPI                        ((SPI_TypeDef*)SPI1_BASE)
#define dm9051_IRQHandler       					(EXTI9_5_IRQHandler)

/* Select SPI DM9051: Chip Select pin low  */
#define SPI_DM9051_CS_LOW()       GPIO_ResetBits(GPIO_DM9051_CS, GPIO_Pin_DM9051_CS); \
																	GPIO_SetBits(GPIOF, GPIO_Pin_10)
/* Deselect SPI DM9051: Chip Select pin high */
#define SPI_DM9051_CS_HIGH()      GPIO_SetBits(GPIO_DM9051_CS, GPIO_Pin_DM9051_CS); \
																	GPIO_ResetBits(GPIOF, GPIO_Pin_10)
																	
/*********************************************************************************************************
* @brief  Sends a byte through the SPI interface and return the byte received from the SPI bus.
* @param  byte : byte to send.
* @retval The value of the received byte.
**********************************************************************************************************/
static __inline u8 SPI_DM9051_SendByte(u8 byte)
{
	/* Loop while DR register in not emplty */
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == 0);

	/* Send byte through the SPI1 peripheral */
	SPI_I2S_SendData(SPI1, byte);

	/* Wait to receive a byte */
	//while (SPI_I2S_GetFlagStatus(DM9051_SPI, SPI_I2S_FLAG_RXNE) == 0);
	SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE);
	
	/* Return the byte read from the SPI bus */
	return SPI_I2S_ReceiveData(SPI1);
}

/* -----------------------------------------------------*/
/* DM9051_Write_Mem()																		*/
/* DM9051 burst write command: SPI_WR_BURST = 0xF8			*/
/*------------------------------------------------------*/
//static 
void DM9051_Write_Mem(u8* pu8data, u16 datalen)
{
	uint32_t i;
	
	// Send the array to the slave
	u8 burstcmd = SPI_WR_BURST;
	
	SPI_DM9051_CS_LOW();

	SPI_DM9051_SendByte(burstcmd);
	
	for(i=0; i< datalen; i++) {
		SPI_DM9051_SendByte(pu8data[i]);
	}
	
	SPI_DM9051_CS_HIGH();
}

/* -----------------------------------------------------*/
/* DM9051_Read_Mem()																		*/
/* DM9051 burst read command: SPI_RD_BURST = 0x72				*/
/*------------------------------------------------------*/
//static 
void DM9051_Read_Mem(u8* pu8data, u16 datalen)
{
	u32 i;
	
	// Read SPI_Data_Array back from the slave
	u8 burstcmd = SPI_RD_BURST;
	
	SPI_DM9051_CS_LOW();
	
	SPI_DM9051_SendByte(burstcmd);
	
	for(i=0; i<datalen; i++) {
		pu8data[i] = SPI_DM9051_SendByte(0x0);
	}
	
	SPI_DM9051_CS_HIGH();

}	

/* -----------------------------------------------------*/
/* DM9051_Read_Reg()																		*/
/* SPI read command: bit7 = 0,													*/
/*                   bit[6:0] = Reg. offset addr				*/
/*------------------------------------------------------*/
//static 
u8 DM9051_Read_Reg(u8 Reg_Off)
{
	u8 spi_data;
	
	SPI_DM9051_CS_LOW();	

	// SPI transfer DM9051 Read-Command and Reg. offset.
	SPI_DM9051_SendByte(Reg_Off);		//Read command + Register offset address

	spi_data = SPI_DM9051_SendByte(0x0);		//Dummy for read register value.
	
	SPI_DM9051_CS_HIGH();
	
	return spi_data;
}

/* -----------------------------------------------------*/
/* DM9051_Write_Reg()																		*/
/* SPI write command: bit7 = 1, 												*/
/*                   bit[6:0] = Reg. offset addr				*/
/*------------------------------------------------------*/
//static 
void DM9051_Write_Reg(u8 Reg_Off, u8 spi_data)
{
	u8 cmdaddr;
	
	cmdaddr = (Reg_Off | 0x80);
	
	SPI_DM9051_CS_LOW();
	
	// SPI transfer DM9051 Read-Command and Reg. offset.
	SPI_DM9051_SendByte(cmdaddr); //Read command + Register offset address
	SPI_DM9051_SendByte(spi_data);
	
	SPI_DM9051_CS_HIGH();
	
	return;
}

/*   SPI Initialization  */
void SPI_Configuration(void)
{
	SPI_InitTypeDef  SPI_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;
#ifdef DM9051_INT_OLD
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
#endif //DM9051_INT_OLD

    /* ------------------------------------------------
   // RCC_APB2Periph_GPIOC is defined as 'RCC_APB2Periph_GPIO_DM9051_CS' (was) (PC0)
   // RCC_APB2Periph_GPIOA is defined as 'RCC_APB2Periph_GPIO_DM9051_CS' (is) (PA4)
    ------------------------------------------------ */
	/* Enable SPI1 and GPIO clocks */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB 
	| RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOF, ENABLE); 
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
	
	/* Configure SPI1 pins: SCK, MISO and MOSI */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* ------------------------------------------------
    //GPIO_Pin_DM9051_CS= GPIO_Pin_0 (PC0) or GPIO_Pin_4 (PA4)
    //GPIO_DM9051_CS= GPIOC (PC0) or GPIOA (PA4)
    ------------------------------------------------ */
	/* Configure I/O for DM9051 Chip select */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_DM9051_CS; 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIO_DM9051_CS, &GPIO_InitStructure); 

	
#if 1
	/* Configure pins: SPI FLASH CS --------------------------------------------*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_SetBits(GPIOB, GPIO_Pin_2);
#endif

#if 1
	/* Configure pins: LCD TOUCH CS --------------------------------------------*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOF, &GPIO_InitStructure);
	GPIO_SetBits(GPIOF, GPIO_Pin_10);
#endif	//0  
	/* Deselect the FLASH: Chip Select high */
	//SPI_DM9051_CS_HIGH();

	/* SPI1 configuration */
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_CRCPolynomial = 7;
	SPI_Init(SPI1, &SPI_InitStructure);
	
	/* Enable SPI1  */
	SPI_Cmd(SPI1, ENABLE);

#ifdef DM9051_INT_OLD  
	/* configure PC7 as external interrupt */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	/* Connect DM9051 EXTI Line to GPIOC Pin 7 */
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource7);

	/* Configure DM9051 EXTI Line to generate an interrupt on falling edge */
	EXTI_InitStructure.EXTI_Line = EXTI_Line7;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);

	/* Clear DM9051 EXTI line pending bit */
	EXTI_ClearITPendingBit(EXTI_Line7);	

	/* Enable the EXTI7 Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);    
#endif	//DM9051_INT_OLD

}
