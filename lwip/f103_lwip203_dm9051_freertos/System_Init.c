/******************** (C) COPYRIGHT 2014 91mcu **************************
* File Name         : system_init.c
* Description       : System Init       
* Platform          : STM32F103ZE
* Libraries version : ST3.5.0
**********************************************************************************/	
#include "stdio.h"
#include "stm32f10x.h"
#include "System_Init.h"
#include "DM9051.h"

#if 0
//[if 0 to check out the DM9051.c DRIVER support functions]

/**
 *
 */
 
/* DM9051 */
#define GPIO_DM9051_CS										GPIOC
#define RCC_APB2Periph_GPIO_DM9051_CS			RCC_APB2Periph_GPIOC
#define GPIO_Pin_DM9051_CS								GPIO_Pin_0
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
.................................................. migrated out ...
void SPI_Configuration(void)
{
	SPI_InitTypeDef  SPI_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	/* Enable SPI1 and GPIO clocks */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB 
	| RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOF, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
	
	/* Configure SPI1 pins: SCK, MISO and MOSI */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* Configure I/O for DM9051 Chip select */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	
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
#endif	//1  
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

#ifdef DM9051_INT_OLD  .................................................. migrated out ...
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
#endif	//DM9051_INT_OLD .................................................. migrated out ...

}
#endif //0

//#define	USART_CONFIG	USART1_Configuration
//#define	PUTCHAR_USART	USART1
//#define	USART_CONFIG	test_USART2_config
//#define	PUTCHAR_USART	USART2
//#define	USART_CONFIG	USART3_Configuration
//#define	PUTCHAR_USART	USART3
#define	USART_CONFIG	USART1_Configuration //test_USART2_config
#define	PUTCHAR_USART	USART1

#if 1
/*******************************************************************************
* Function Name  : USART1_Configuration
* Description    : Configure USART1 (RCC_APB2Periph_USART1)
* Input          : None
* Output         : None
* Return         : None
* Attention		 : None
*******************************************************************************/
void USART1_Configuration(void)
{ 
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure; 

	RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1,ENABLE);  //PA.9  //PA.10
	/*
*  USART1_TX -> PA9 , USART1_RX ->	PA10
*/				
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;	         
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
	GPIO_Init(GPIOA, &GPIO_InitStructure);		   

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;	        
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;  
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

	USART_Init(USART1, &USART_InitStructure); 
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
	USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
	USART_ClearFlag(USART1,USART_FLAG_TC);
	USART_Cmd(USART1, ENABLE);
}
#endif

/*******************************************************************************
* Function Name  : USART_Configuration
* Description    : Configure basic USART2 ( i.e. AT_USART_ ) 
*******************************************************************************/
void test_USART2_config(void)
{
	//(at_type.baudrate, at_type.wordlen, at_type.parity, at_type.stop)
	// AT_USART_Configuration(115200, 8, 0, 1); //partial=
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	
//System_Initialization(); //System initial= 
//:
	#if 0
	/* Note: If both GPIOx and USARTn are in the same APBn, Then can configure in the same function call */
	/*
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_USART1, ENABLE);
	*/
	#endif

	//RCC_Configuration(); partial=
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE); // <-- USART2 'while-op' avoid hang
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE); // <--- USART2 GPIO_Pin_2 & GPIO_Pin_3 ebable it OK.
	/* APB1 periph_USART2 & */
	/* APB2 periph_GPIOA */

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	USART_InitStructure.USART_BaudRate = 115200; //9600;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART2, &USART_InitStructure); 
	USART_Cmd(USART2, ENABLE); // NOT USART1
}

/*******************************************************************************
* Function Name  : USART3_Configuration
* Description    : Configure USART3 (RCC_APB1Periph_USART3)
*******************************************************************************/
void USART3_Configuration(void)
{ 
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure; 

	RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOB | RCC_APB1Periph_USART3,ENABLE);  //PB.10  //PB.11	
/**
 *  USART3_TX -> PB10 , USART3_RX ->	PB11
 */	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10; //PB.10      
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
	GPIO_Init(GPIOA, &GPIO_InitStructure);		   

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11; //PB.11       
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;  
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	USART_InitStructure.USART_BaudRate = 115200;//串口波特率
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

	USART_Init(USART3, &USART_InitStructure); //初始化串口
	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);//开启串口接收中断
	USART_ITConfig(USART3, USART_IT_TXE, ENABLE);//开启串口 "TX" ..
	USART_ClearFlag(USART3,USART_FLAG_TC);//"Clear" ..
	USART_Cmd(USART3, ENABLE);//使能串口 
}

#ifdef __GNUC__

 .m,nvmkln k nkjwer...
/* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
	set to 'Yes') calls __io_putchar() */
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else

 //sdklmklnsdvklnklsvklsl;...
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

/**
* @brief  Retargets the C library printf function to the USART.
* @param  None
* @retval None
*/
PUTCHAR_PROTOTYPE
{
	/* Place your implementation of fputc here */
	/* e.g. write a character to the USART */
	USART_SendData(PUTCHAR_USART, (uint8_t) ch); //USART1 or USART3

	/* Loop until the end of transmission */
	while (USART_GetFlagStatus(PUTCHAR_USART, USART_FLAG_TC) == RESET) //USART1 or USART3
	{}

#if 0
	//=
	/* Transmit Data, USART1 or USART3 */
	while((PUTCHAR_USART->SR&0X40)==0);//循环发送,直到发送完毕   
  PUTCHAR_USART->DR = (uint16_t)ch & (uint16_t)0x01FF;
#endif
	return ch;
}

void GPIO_Configuration(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOF , ENABLE); 						 
	/**
		*  LED1 -> PF6 , LED2 -> PF7 , LED3 -> PF8 , LED4 -> PF9
		*/					 
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOF, &GPIO_InitStructure);	
	
	GPIO_ResetBits(GPIOF,GPIO_Pin_8);
}

/* system Configuration */
void System_Initialization(void)
{
	/* UART Configuration */
	USART_CONFIG(); 
	//test_USART2_config();
	//USART1_Configuration(); USART3_Configuration();
	//USART_Configuration1(); or USART_Configuration3();

	/* SPI Configuration */
	SPI_Configuration();
	Interrupt_Configuration();
	
	/* GPIO Configuration */
	GPIO_Configuration();
}
