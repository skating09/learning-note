/*
*********************************************************************************************************
*	                                  
*	模块名称 : 主函数
*	文件名称 : main.c
*	版    本 : V2.0
*	说    明 : FreeRTOS基础教程
*	修改记录 :
*		版本号   日期        作者             说明
*		v1.0    2013-09-10  Eric2013  创建该文件，ST固件库版本为V1.0.2
*		V2.0    2013-02-24  Eric2013  升级FreeRTOS到V8.0.0，升级F4固件库到1.3.0 
*
*	Copyright (C), 2013-2014, 安富莱电子 www.armfly.com
*
*********************************************************************************************************
*/
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "croutine.h"
#include "stdio.h"
#include "semphr.h"
#include "netconf.h"
#include "lwip/dhcp.h"
#include "lwip/prot/dhcp.h"
#include "lwip/netifapi.h"
#include "tcpip.h"
#include "ethernetif.h"
#include "main.h"
#include "app_order.h"
#include "app_order_extern.h"
#if 0
#include "port/app/netconn_tcp_client/tcpapp.h"
#endif
#if 0
#include "port/app/socket_tcp_client/app_canaan_tcp/tcp_test.h" 
#endif
#include "port/app/netconn_http_server/httpserver-netconn.h"
#if 0
#include "port/app/dhcp_client/dhcp_client.h"
#endif
#if 0
#include "port/app/netconn_tcp_server/app_tcps/tcpserver-netconn.h" //app_tcps/netconn_tcp_server
#endif
#include "System_Init.h"
#include "DM9051.h"

extern xSemaphoreHandle s_xSemaphore1;
/*
**********************************************************************************************************
											函数声明
**********************************************************************************************************
*/
static void AppTaskCreate (void);
static void AppTaskObject (void);
//extern void vSetupTimerTest( void );
/*
**********************************************************************************************************
											变量声明
**********************************************************************************************************
*/
xTaskHandle xHandleTask4;
xQueueHandle xQueue;

xSemaphoreHandle tsk_xSemaphore;

#define TCP_CLIENT

extern void tt_tcp_server_tt_sem_signal(void); // to call to let server run

/*
*********************************************************************************************************
*	函 数 名: main
*	功能说明: 标准c程序入口。
*	形    参：无
*	返 回 值: 无
*********************************************************************************************************
*/
int main(void)
{
		System_Initialization();
	
#if NO_SYS | 1	
#define COOLNET_MAJ_VER	0
#define COOLNET_MIN_VER	0
#define COOLNET_EXT_VER	0
		printf("\r\n");
		printf("******************************************\r\n");
		printf("***  CoolNet_lwip2.0.3 No_SYS=%d v%d.%d.%d   ***\r\n", NO_SYS, COOLNET_MAJ_VER, COOLNET_MIN_VER, COOLNET_EXT_VER);
		printf("******************************************\r\n");
		printf("***  NO_SYS %d                          ***\r\n", NO_SYS);
		printf("******************************************\r\n");
		printf("***  LWIP_TCPIP_CORE_LOCKING %d         ***\r\n", LWIP_TCPIP_CORE_LOCKING);
		printf("***  TCP_OVERSIZE %d                    ***\r\n", TCP_OVERSIZE);
		printf("\r\n");
#endif	
	
		LwIP_Init();
//LwIP_Init(void);= LwIP_DHCP_Init() or LwIP_FIXED_IP_Init().
	
#if 0
#ifdef USE_DHCP
//LwIP_DHCP_Init();=
		tcpip_init( NULL, NULL );	 // --> sys_thread_new() --> xTaskCreate( (TaskFunction_t) tcpip_thread, name, ...
		{
			struct ip4_addr ipaddr;
			struct ip4_addr netmask;
			struct ip4_addr gw;
			ipaddr.addr = 0;
			netmask.addr = 0;
			gw.addr = 0;
		/* - netif_add(struct netif *netif, struct ip_addr *ipaddr,
							struct ip_addr *netmask, struct ip_addr *gw,
							void *state, err_t (* init)(struct netif *netif),
							err_t (* input)(struct pbuf *p, struct netif *netif))
			
		 Adds your network interface to the netif_list. Allocate a struct
		netif and pass a pointer to this structure as the first argument.
		Give pointers to cleared ip_addr structures when using DHCP,
		or fill them with sane numbers otherwise. The state pointer may be NULL.

		The init function pointer must point to a initialization function for
		your ethernet netif interface. The following code illustrates it's use.*/

			//=netif_add(&xnetif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &tcpip_input);
			netif_input_add(&xnetif, &ipaddr, &netmask, &gw, NULL, &tcpip_input);
			if (1) //(ethernetif_init(&xnetif) == ERR_OK)
			{
				//ethernetif_init.s(&xnetif)
				ethernetif_begining_init(&xnetif);
				//ethernetif_init.m(&xnetif)
				low_level_begin_init(&xnetif); /* initialize the hardware */
					#if 1
						// Now (that is) use as "ethernetif.c"?
					if (s_xSemaphore == NULL){ /* create binary semaphore used for informing ethernetif of frame reception */
						s_xSemaphore= xSemaphoreCreateCounting(20,0);
					}
					xTaskCreate(ethernetif_input, (char*) "Eth_if", ( 350 ), NULL, /* xTaskCreate that handles the ETH_MAC */
										( configMAX_PRIORITIES - 1 ), NULL); /* netifINTERFACE_TASK_STACK_SIZE = (350) */
					#endif
					#if 0
						// How (why not) use "sys_arch.c"?
						sys_thread_new((char*) "Eth_if", ethernetif_input, NULL, TCPIP_THREAD_STACKSIZE, TCPIP_THREAD_PRIO);
					#endif
				//ethernetif_init.e(&xnetif)
				ethernetif_ending_arptimer(); /* arp_timer */
				
				netif_ok_ending_add(&xnetif, &ipaddr, &netmask, &gw);
			}
		}

	 /*  Registers the default network interface. */
		netif_set_default(&xnetif);

	 /*  When the netif is fully configured this function must be called.*/
		netif_set_up(&xnetif); 
#else
//LwIP_FIXED_IP_Init();=
		tcpip_init( NULL, NULL );	
		{
			struct ip4_addr ipaddr;
			struct ip4_addr netmask;
			struct ip4_addr gw;
			IP4_ADDR(&ipaddr, IP_ADDR0, IP_ADDR1, IP_ADDR2, IP_ADDR3);
			IP4_ADDR(&netmask, NETMASK_ADDR0, NETMASK_ADDR1 , NETMASK_ADDR2, NETMASK_ADDR3);
			IP4_ADDR(&gw, GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
		/* - netif_add(struct netif *netif, struct ip_addr *ipaddr,
							struct ip_addr *netmask, struct ip_addr *gw,
							void *state, err_t (* init)(struct netif *netif),
							err_t (* input)(struct pbuf *p, struct netif *netif))
			
		 Adds your network interface to the netif_list. Allocate a struct
		netif and pass a pointer to this structure as the first argument.
		Give pointers to cleared ip_addr structures when using DHCP,
		or fill them with sane numbers otherwise. The state pointer may be NULL.

		The init function pointer must point to a initialization function for
		your ethernet netif interface. The following code illustrates it's use.*/

			netif_add(&xnetif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &tcpip_input);
		}

	 /*  Registers the default network interface. */
		netif_set_default(&xnetif);

	 /*  When the netif is fully configured this function must be called.*/
		netif_set_up(&xnetif); 
		//.Show_Config();
#endif //USE_DHCP
#endif
		
		AppTaskCreate();
		AppTaskObject();
		
	/*
	 * Make single thread demo control, start-up one only ...
	 */
#if 1
		http_server_netconn_init();
#endif
	/*
	 * Make semepher control, start-up order control ...
	 */
#if 0
		tcp_server_init();  	// 1st thread, waiting for next generated thread.
#endif
#if 0
		#ifdef TCP_CLIENT
		//Stone add for TCP client app		
		sys_thread_new("tcpapp_thread", tcpapp_thread, NULL, DEFAULT_THREAD_STACKSIZE, tskIDLE_PRIORITY + 5);
		#else
		tcp_test_init();			// 2nd thread, waiting for next generated thread.
		#endif
#else
		//tt_tcp_server_tt_sem_signal();
#endif
#if 0
		dhcp_client_init(); 	// 3rd thread (must be finished(or time-out) to let previous threads start-run. )
#endif
	
		vTaskStartScheduler();

		/* 
			 If all is well we will never reach here as the scheduler will now be
		 running.  If we do reach here then it is likely that there was insufficient
		 heap available for the idle task to be created. 
		*/
		while (1)
		{
    } 
}

/*
*********************************************************************************************************
*	函 数 名: vTask1
*	功能说明: emwin任务	
*	形    参：pvParameters 是在创建该任务时传递的形参
*	返 回 值: 无
*********************************************************************************************************
*/
void vTask1( void *pvParameters )
{
	
    while(1)
    {
		//MainTask();	
			
		//	printf("renwuyunxing\r\n");
    vTaskDelay( 200 );		//后加入	
	}
}

/*
*********************************************************************************************************
*	函 数 名: vTask2
*	功能说明:触摸屏
*	形    参：pvParameters 是在创建该任务时传递的形参
*	返 回 值: 无
*********************************************************************************************************
*/
void vTask2( void *pvParameters )
{
	portTickType xLastWakeTime;
  const portTickType xFrequency = 10;

	// Initialise the xLastWakeTime variable with the current time.
  xLastWakeTime = xTaskGetTickCount();
	
	while (1) 
	{
		vTaskDelayUntil( &xLastWakeTime, xFrequency );
	}
}

/*
*********************************************************************************************************
*	函 数 名: vTask3
*	功能说明: 按键扫描	
*	形    参：pvParameters 是在创建该任务时传递的形参
*	返 回 值: 无
*********************************************************************************************************
*/
void vTask3( void *pvParameters )
{   
		//portTickType xLastWakeTime;
    //const portTickType xFrequency = 5;
	
    //xLastWakeTime = xTaskGetTickCount();
	
    while(1){
			//vTaskDelayUntil( &xLastWakeTime, xFrequency );
			xSemaphoreGive( tsk_xSemaphore );
    }
}

/*
*********************************************************************************************************
*	函 数 名: vTask4
*	功能说明: LED闪烁		
*	形    参：pvParameters 是在创建该任务时传递的形参
*	返 回 值: 无
*********************************************************************************************************
*/
u8 flg_INT = 0;
void vTask4( void *pvParameters )
{
    while(1){
	
#ifndef DM9051_INT
			flg_INT = 1; //non-interrupt [test] as polling... // INT ................................... no used..
#endif //DM9051_INT
		
			//printf("renwuyunxing7\r\n");
		
			/* ISR do give semaphore / E.g. DM9051_ISR() --> give semaphore */
			xSemaphoreTake( tsk_xSemaphore, portMAX_DELAY);
		
			
			if (DM9051_Interface_Check()) // hint: low_level_linking_monitor_check()
			{
			if (flg_INT)
			{
				printf("%d", flg_INT); //test
				flg_INT = 0;
				printf("%d", flg_INT); // test
				DM9051_RXI_DIS(); 
				if (DM9051_RXBYTE()) // hint: low_level_interrupt_to_rxbyte_check()
					xSemaphoreGive( s_xSemaphore1 );
				else
					DM9051_RXI_EN(); //hint: low_level_input_rxbyte_check_end();
			}
			}
    }
}

/*
*********************************************************************************************************
*	函 数 名: AppTaskCreate
*	功能说明: 创建应用任务
*	形    参：无
*	返 回 值: 无
*********************************************************************************************************
*/
static void AppTaskCreate (void)
{
#if 0
	/* Create one task. */
    xTaskCreate(    vTask1,     /* Pointer to the function that implements the task.              */
                    "Task 1",   /* Text name for the task.  This is to facilitate debugging only. */
                    500,        /* Stack depth in words.                                          */
                    NULL,       /* We are not using the task parameter.                           */
                    1,          /* This task will run at priority 1.                              */
                    NULL );     /* We are not using the task handle.                              */

    /* Create one task. */
    xTaskCreate(    vTask2,     /* Pointer to the function that implements the task. */
                    "Task 2",   /* Text name for the task.  This is to facilitate debugging only. */
                    500,        /* Stack depth in words.                                          */
                    NULL,       /* We are not using the task parameter.                           */
                    2,          /* This task will run at priority 2.                              */
                    NULL );     /* We are not using the task handle.                              */
#endif //0	
	 /* Create one task. */
    xTaskCreate(    vTask3,     /* Pointer to the function that implements the task. */
                    "Task 3",   /* Text name for the task.  This is to facilitate debugging only. */
                    500,        /* Stack depth in words.                                          */
                    NULL,       /* We are not using the task parameter.                           */
                    4,          /* This task will run at priority 2.                              */
                    NULL);    
	
	/* Create one task. */
    xTaskCreate(    vTask4,     /* Pointer to the function that implements the task. */
                    "Task 4",   /* Text name for the task.  This is to facilitate debugging only. */
                    500,        /* Stack depth in words.                                          */
                    NULL,       /* We are not using the task parameter.                           */
                    5,          /* This task will run at priority 2.                              */
                    &xHandleTask4 );     /* We are not using the task handle.                              */				
										
}

/*
*********************************************************************************************************
*	函 数 名: AppTaskObject
*	功能说明: 创建应用任务
*	形    参：无
*	返 回 值: 无
*********************************************************************************************************
*/
static void AppTaskObject (void)
{ 
   vSemaphoreCreateBinary( tsk_xSemaphore );

   xSemaphoreTake( tsk_xSemaphore, portMAX_DELAY);
}

