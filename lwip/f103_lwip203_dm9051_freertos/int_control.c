#include "stdio.h"
#include "stm32f10x.h"
#include "DM9051.h"

#if 0
/*   SPI Initialization  */
# declare void Interrupt_Configuration(void) function proto-type in System_Init.h

# call Interrupt_Configuration(void) function in "System_Initialization(void)" in System_Init.c

# implement void Interrupt_Configuration(void) function in this file int_control.c
//void Interrupt_Configuration(void)
//{
//}
#endif

extern u8 flg_INT;

/*********************************************************************************************************
* @brief  Interrupt Service Routine (ISR).
* @param  void.
* @retval void.
* Remark: In Startup ("startup_stm32f10x_hd.s")
* Locate the '__Vectors', it put ISR function name in the vector ranking entry.
*   DCD     EXTI9_5_IRQHandler         ; EXTI Line 9..5 
**********************************************************************************************************/
void EXTI9_5_IRQHandler(void)
{
#if 0	
	OS_ENTER_CRITICAL(); /* Tell uC/OS-II that we are starting an ISR */
	OSIntNesting++;
	OS_EXIT_CRITICAL();
	if (OSIntNesting > 0)
		OSIntNesting--;
#endif

#ifdef DM9051_INT
	if (EXTI_GetITStatus(EXTI_Line7) != RESET)
	{
		/*
		uint8_t temp = DM9051_Read_Reg(DM9051_ISR);
		DM9051_Write_Reg(DM9051_ISR, temp);
		*/
		EXTI_ClearITPendingBit(EXTI_Line7);
		flg_INT = 1;
	}
	
	/*
	EXTI_ClearITPendingBit(EXTI_Line7);
	while( GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_7)==0 )
	{
		DM9051_isr();
	}
	*/
#endif //DM9051_INT
	
#if 0	
	OSIntExit(); /* Tell uC/OS-II that we are leaving the ISR */
#endif
}

/*   Interrupt Initialization  */
/**
  * configure PC7 as external interrupt
  * Connect DM9051 EXTI Line to GPIOC Pin 7 
  * Configure DM9051 EXTI Line to generate an interrupt on falling edge
  * Clear DM9051 EXTI line pending bit
  * Enable the EXTI7 Interrupt
  */
void Interrupt_Configuration(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
#ifdef DM9051_INT
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
#endif //DM9051_INT
	
#ifdef DM9051_INT
	#if 1
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	#endif
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
	#if 1
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
	#endif
	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);  
#endif //DM9051_INT
}
