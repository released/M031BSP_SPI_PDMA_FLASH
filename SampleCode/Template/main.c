/*_____ I N C L U D E S ____________________________________________________*/
#include <stdio.h>
#include <string.h>
#include "NuMicro.h"

#include	"project_config.h"

#include	"SPI_Flash.h"
/*_____ D E C L A R A T I O N S ____________________________________________*/

uint8_t TxBuffer[SPI_FLASH_PAGE_BYTE] = {0};
uint8_t RxBuffer[SPI_FLASH_PAGE_BYTE] = {0};

uint8_t Tx4KBuffer[SPI_FLASH_SECTOR_SIZE] = {0};
uint8_t Rx4KBuffer[SPI_FLASH_SECTOR_SIZE] = {0};

uint8_t SPI_FLASH_page_counter = 0;


/*_____ D E F I N I T I O N S ______________________________________________*/
volatile uint32_t BitFlag = 0;
volatile uint32_t counter_tick = 0;

/*_____ M A C R O S ________________________________________________________*/

/*_____ F U N C T I O N S __________________________________________________*/

void tick_counter(void)
{
	counter_tick++;
}

uint32_t get_tick(void)
{
	return (counter_tick);
}

void set_tick(uint32_t t)
{
	counter_tick = t;
}

void compare_buffer(uint8_t *src, uint8_t *des, int nBytes)
{
    uint16_t i = 0;	
	
    for (i = 0; i < nBytes; i++)
    {
        if (src[i] != des[i])
        {
            printf("error idx : %4d : 0x%2X , 0x%2X\r\n", i , src[i],des[i]);
			set_flag(flag_error , ENABLE);
        }
    }

	if (!is_flag_set(flag_error))
	{
    	printf("%s finish \r\n" , __FUNCTION__);	
		set_flag(flag_error , DISABLE);
	}

}

void reset_buffer(uint8_t *pucBuff, int nBytes)
{
	#if 1
    uint16_t i = 0;	
    for ( i = 0; i < nBytes; i++)
    {
        pucBuff[i] = 0x00;
    }	
	#else	//extra 20 bytes , with <string.h>
	memset(pucBuff, 0, nBytes * (sizeof(pucBuff[0]) ));
	#endif
}

void copy_buffer(void *dest, void *src, unsigned int size)
{
    uint8_t *pu8Src, *pu8Dest;
    unsigned int i;
    
    pu8Dest = (uint8_t *)dest;
    pu8Src  = (uint8_t *)src;


	#if 0
	  while (size--)
	    *pu8Dest++ = *pu8Src++;
	#else
    for (i = 0; i < size; i++)
        pu8Dest[i] = pu8Src[i];
	#endif
}

void dump_buffer(uint8_t *pucBuff, int nBytes)
{
    uint16_t i = 0;
    
    printf("dump_buffer : %2d\r\n" , nBytes);    
    for (i = 0 ; i < nBytes ; i++)
    {
        printf("0x%2X," , pucBuff[i]);
        if ((i+1)%8 ==0)
        {
            printf("\r\n");
        }            
    }
    printf("\r\n\r\n");
}

void  dump_buffer_hex(uint8_t *pucBuff, int nBytes)
{
    int     nIdx, i;

    nIdx = 0;
    while (nBytes > 0)
    {
        printf("0x%04X  ", nIdx);
        for (i = 0; i < 16; i++)
            printf("%02X ", pucBuff[nIdx + i]);
        printf("  ");
        for (i = 0; i < 16; i++)
        {
            if ((pucBuff[nIdx + i] >= 0x20) && (pucBuff[nIdx + i] < 127))
                printf("%c", pucBuff[nIdx + i]);
            else
                printf(".");
            nBytes--;
        }
        nIdx += 16;
        printf("\n");
    }
    printf("\n");
}

void delay(uint16_t dly)
{
/*
	delay(100) : 14.84 us
	delay(200) : 29.37 us
	delay(300) : 43.97 us
	delay(400) : 58.5 us	
	delay(500) : 73.13 us	
	
	delay(1500) : 0.218 ms (218 us)
	delay(2000) : 0.291 ms (291 us)	
*/

	while( dly--);
}


void delay_ms(uint16_t ms)
{
	TIMER_Delay(TIMER0, 1000*ms);
}


void GPIO_Init (void)
{
    SYS->GPB_MFPH = (SYS->GPB_MFPH & ~(SYS_GPB_MFPH_PB14MFP_Msk)) | (SYS_GPB_MFPH_PB14MFP_GPIO);
    SYS->GPB_MFPH = (SYS->GPB_MFPH & ~(SYS_GPB_MFPH_PB15MFP_Msk)) | (SYS_GPB_MFPH_PB15MFP_GPIO);
	
    GPIO_SetMode(PB, BIT14, GPIO_MODE_OUTPUT);
    GPIO_SetMode(PB, BIT15, GPIO_MODE_OUTPUT);	
}


void TMR1_IRQHandler(void)
{
//	static uint32_t LOG = 0;

	
    if(TIMER_GetIntFlag(TIMER1) == 1)
    {
        TIMER_ClearIntFlag(TIMER1);
		tick_counter();

		if ((get_tick() % 1000) == 0)
		{
//        	printf("%s : %4d\r\n",__FUNCTION__,LOG++);
			PB14 ^= 1;
		}

		if ((get_tick() % 50) == 0)
		{

		}	
    }
}


void TIMER1_Init(void)
{
    TIMER_Open(TIMER1, TIMER_PERIODIC_MODE, 1000);
    TIMER_EnableInt(TIMER1);
    NVIC_EnableIRQ(TMR1_IRQn);	
    TIMER_Start(TIMER1);
}

void UARTx_Process(void)
{
	uint8_t res = 0;
    uint16_t i = 0;
	static uint8_t cnt = 0;
    uint16_t page_cnt = 0;
    uint16_t sector_cnt = 0;
	
	res = UART_READ(UART0);

	if (res == 'x' || res == 'X')
	{
		NVIC_SystemReset();
	}

	if (res > 0x7F)
	{
		printf("invalid command\r\n");
	}
	else
	{
		switch(res)
		{
			case '?':
				#if (_debug_log_UART_ == 1)	//debug
				printf("\r\n==========================\r\n");
				
				printf("1: chip erase\r\n");
				printf("2: fill in TX data\r\n");
				printf("3: page counter\r\n");
				printf("4: Page write\r\n");
				printf("5: Page read\r\n");		
				printf("6: self test , write , read , compare\r\n");	
				printf("7: self test , write , read , compare  , with PDMA\r\n");
				
				printf("8: read ID (0x90)\r\n");	
				
				printf("==========================\r\n\r\n");
				#endif
				break;	

		
			case '1':
				#if (_debug_log_UART_ == 1)	//debug
				printf("perform SpiFlash_ChipErase\r\n");
				#endif
				
			    /* Erase SPI flash */
			    SpiFlash_ChipErase();

			    /* Wait ready */
			    SpiFlash_WaitReady();

				#if (_debug_log_UART_ == 1)	//debug
				printf("erase finish\r\n\r\n");
				#endif
				break;	

			case '2':
				#if (_debug_log_UART_ == 1)	//debug				
				printf("increase test data start from 0x%2X\r\n" , cnt);
				#endif

				#if defined (TEST_SPI_PAGE)
				//reset TxBuffer
				reset_buffer(TxBuffer,SPI_FLASH_PAGE_BYTE);

				//fill in data
			    for ( i = 0; i < SPI_FLASH_PAGE_BYTE; i++)
			    {
			        TxBuffer[i] = 0x00 + i + cnt;
			    }

				#if (_debug_log_UART_ == 1)	//debug
				printf("TxBuffer : \r\n");	
				dump_buffer_hex(TxBuffer,SPI_FLASH_PAGE_BYTE);				
				#endif	

				#elif defined (TEST_SPI_SECTOR)
				//reset TxBuffer
				reset_buffer(Tx4KBuffer,SPI_FLASH_SECTOR_SIZE);

				//fill in data
			    for ( i = 0; i < SPI_FLASH_SECTOR_SIZE; i++)
			    {
			        Tx4KBuffer[i] = 0x00 + i + cnt;
			    }

				#if (_debug_log_UART_ == 1)	//debug
				printf("Tx4KBuffer : \r\n");	
				dump_buffer_hex(Tx4KBuffer,SPI_FLASH_SECTOR_SIZE);				
				#endif	

				#endif				
				cnt++; 
			
				break;

			case '3':
				#if (_debug_log_UART_ == 1)	//debug
				printf("SPI_FLASH_page_counter current : 0x%2X\r\n\r\n" ,SPI_FLASH_page_counter++);	
				#endif
			
				break;

			case '4':
				#if (_debug_log_UART_ == 1)	//debug
				printf("programming...(%d)\r\n",SPI_FLASH_page_counter);
				#endif

				#if defined (TEST_SPI_PAGE)				
//				SpiFlash_PageWrite(SPI_FLASH_page_counter,TxBuffer,SPI_FLASH_PAGE_BYTE,DISABLE);
				SpiFlash_PageWrite(SPI_FLASH_page_counter,TxBuffer,SPI_FLASH_PAGE_BYTE,ENABLE);

				#elif defined (TEST_SPI_SECTOR)
//				SpiFlash_SectorWrite(SPI_FLASH_page_counter,Tx4KBuffer,SPI_FLASH_SECTOR_SIZE,DISABLE);
				SpiFlash_SectorWrite(SPI_FLASH_page_counter,Tx4KBuffer,SPI_FLASH_SECTOR_SIZE,ENABLE);

				#endif
				
				#if (_debug_log_UART_ == 1)	//debug
				printf("programming finish\r\n\r\n");
				#endif
				
				break;

			case '5':
				#if (_debug_log_UART_ == 1)	//debug
				printf("read page ...(%2d)\r\n",SPI_FLASH_page_counter);
				#endif

				#if defined (TEST_SPI_PAGE)	
				//reset RxBuffer
			    reset_buffer(RxBuffer,SPI_FLASH_PAGE_BYTE);
				
//				SpiFlash_PageRead(SPI_FLASH_page_counter,RxBuffer,SPI_FLASH_PAGE_BYTE,DISABLE);
				SpiFlash_PageRead(SPI_FLASH_page_counter,RxBuffer,SPI_FLASH_PAGE_BYTE,ENABLE);
				
				#if (_debug_log_UART_ == 1)	//debug				
				dump_buffer_hex(RxBuffer,SPI_FLASH_PAGE_BYTE);
				printf("read page finish\r\n\r\n");	
				#endif

				#elif defined (TEST_SPI_SECTOR)
				//reset RxBuffer
			    reset_buffer(Rx4KBuffer,SPI_FLASH_SECTOR_SIZE);
				
//				SpiFlash_SectorRead(SPI_FLASH_page_counter,Rx4KBuffer,SPI_FLASH_SECTOR_SIZE,DISABLE);
				SpiFlash_SectorRead(SPI_FLASH_page_counter,Rx4KBuffer,SPI_FLASH_SECTOR_SIZE,ENABLE);
				
				#if (_debug_log_UART_ == 1)	//debug				
				dump_buffer_hex(Rx4KBuffer,SPI_FLASH_SECTOR_SIZE);
				printf("read page finish\r\n\r\n");	
				#endif

				#endif
				
				break;				

			case '6':
				#if (_debug_log_UART_ == 1)	//debug
				printf("perform SpiFlash_ChipErase\r\n");
				#endif
				
			    /* Erase SPI flash */
			    SpiFlash_ChipErase();

			    /* Wait ready */
			    SpiFlash_WaitReady();

				#if (_debug_log_UART_ == 1)	//debug
				printf("erase finish\r\n\r\n");
				#endif

				for ( page_cnt = 0 ; page_cnt < TEST_NUMBER ; page_cnt++)
				{
					#if (_debug_log_UART_ == 1)	//debug
					printf("\r\nSELF TEST ... (idx : %2d)\r\n" , page_cnt);
					#endif

					//reset RxBuffer
				    reset_buffer(RxBuffer,SPI_FLASH_PAGE_BYTE);

					//reset TxBuffer
					reset_buffer(TxBuffer,SPI_FLASH_PAGE_BYTE);

					//fill in data
				    for ( i = 0; i < SPI_FLASH_PAGE_BYTE; i++)
				    {
				        TxBuffer[i] = 0x00 + i + cnt;
				    }

//					printf("\r\nTxBuffer : \r\n");
//					dump_buffer_hex(TxBuffer,SPI_FLASH_PAGE_BYTE);				
					cnt++;
					
					SpiFlash_PageWrite(page_cnt,TxBuffer,SPI_FLASH_PAGE_BYTE,DISABLE);
					SpiFlash_PageRead(page_cnt,RxBuffer,SPI_FLASH_PAGE_BYTE,DISABLE);

//					printf("\r\nRxBuffer\r\n");
//					dump_buffer_hex(RxBuffer,SPI_FLASH_PAGE_BYTE);		

					compare_buffer(TxBuffer,RxBuffer,SPI_FLASH_PAGE_BYTE);
				}

				#if (_debug_log_UART_ == 1)	//debug
				printf("SELF TEST finish\r\n\r\n");	
				#endif
			
				break;	


			case '7':

				#if (_debug_log_UART_ == 1)	//debug
				printf("perform SpiFlash_ChipErase\r\n");
				#endif
				
			    /* Erase SPI flash */
			    SpiFlash_ChipErase();

			    /* Wait ready */
			    SpiFlash_WaitReady();

				#if (_debug_log_UART_ == 1)	//debug
				printf("erase finish\r\n\r\n");
				#endif

				for ( page_cnt = 0 ; page_cnt < TEST_NUMBER ; page_cnt++)
				{
					#if (_debug_log_UART_ == 1)	//debug
					printf("\r\nPDMA SELF TEST ... (idx : %2d)\r\n" , page_cnt);
					#endif

					//reset RxBuffer
				    reset_buffer(RxBuffer,SPI_FLASH_PAGE_BYTE);

					//reset TxBuffer
					reset_buffer(TxBuffer,SPI_FLASH_PAGE_BYTE);

					//fill in data
				    for ( i = 0; i < SPI_FLASH_PAGE_BYTE; i++)
				    {
				        TxBuffer[i] = 0x00 + i + cnt;
				    }

//					printf("\r\nTxBuffer : \r\n");
//					dump_buffer_hex(TxBuffer,SPI_FLASH_PAGE_BYTE);				
					cnt++;
					
					SpiFlash_PageWrite(page_cnt,TxBuffer,SPI_FLASH_PAGE_BYTE,ENABLE);
					SpiFlash_PageRead(page_cnt,RxBuffer,SPI_FLASH_PAGE_BYTE,ENABLE);

//					printf("\r\nRxBuffer\r\n");
//					dump_buffer_hex(RxBuffer,SPI_FLASH_PAGE_BYTE);		

					compare_buffer(TxBuffer,RxBuffer,SPI_FLASH_PAGE_BYTE);
				}

				#if (_debug_log_UART_ == 1)	//debug
				printf("PDMA SELF TEST finish\r\n\r\n");	
				#endif

				break;

			case '8':			
				i = SpiFlash_ReadMidDid();
				#if (_debug_log_UART_ == 1)	//debug
				printf("SpiFlash_ReadMidDid : 0x%2X\r\n\n" , i);
				#endif
				
				break;		
			
			case 'X':
			case 'x':
			case 'Z':
			case 'z':
				NVIC_SystemReset();
			
				break;		
			
		}
	}
}

void UART02_IRQHandler(void)
{

    if(UART_GET_INT_FLAG(UART0, UART_INTSTS_RDAINT_Msk | UART_INTSTS_RXTOINT_Msk))     /* UART receive data available flag */
    {
        while(UART_GET_RX_EMPTY(UART0) == 0)
        {
            UARTx_Process();
        }
    }

    if(UART0->FIFOSTS & (UART_FIFOSTS_BIF_Msk | UART_FIFOSTS_FEF_Msk | UART_FIFOSTS_PEF_Msk | UART_FIFOSTS_RXOVIF_Msk))
    {
        UART_ClearIntFlag(UART0, (UART_INTSTS_RLSINT_Msk| UART_INTSTS_BUFERRINT_Msk));
    }	
}

void UART0_Init(void)
{
    SYS_ResetModule(UART0_RST);

    /* Configure UART0 and set UART0 baud rate */
    UART_Open(UART0, 115200);
    UART_EnableInt(UART0, UART_INTEN_RDAIEN_Msk | UART_INTEN_RXTOIEN_Msk);
    NVIC_EnableIRQ(UART02_IRQn);
	
	#if (_debug_log_UART_ == 1)	//debug
	printf("\r\nCLK_GetCPUFreq : %8d\r\n",CLK_GetCPUFreq());
	printf("CLK_GetHXTFreq : %8d\r\n",CLK_GetHXTFreq());
	printf("CLK_GetLXTFreq : %8d\r\n",CLK_GetLXTFreq());	
	printf("CLK_GetPCLK0Freq : %8d\r\n",CLK_GetPCLK0Freq());
	printf("CLK_GetPCLK1Freq : %8d\r\n",CLK_GetPCLK1Freq());	
	#endif	

}

void SYS_Init(void)
{
    /* Unlock protected registers */
    SYS_UnlockReg();

    CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk);
    CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk);

//    CLK_EnableXtalRC(CLK_PWRCTL_HXTEN_Msk);
//    CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk);

//    CLK_EnableXtalRC(CLK_PWRCTL_LIRCEN_Msk);
//    CLK_WaitClockReady(CLK_STATUS_LIRCSTB_Msk);	

//    CLK_EnableXtalRC(CLK_PWRCTL_LXTEN_Msk);
//    CLK_WaitClockReady(CLK_STATUS_LXTSTB_Msk);	

    /* Select HCLK clock source as HIRC and HCLK source divider as 1 */
    CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HIRC, CLK_CLKDIV0_HCLK(1));

    CLK_EnableModuleClock(UART0_MODULE);
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART0SEL_HIRC, CLK_CLKDIV0_UART0(1));

    CLK_EnableModuleClock(PDMA_MODULE);

	CLK_EnableModuleClock(TMR0_MODULE);
	CLK_SetModuleClock(TMR0_MODULE, CLK_CLKSEL1_TMR0SEL_HIRC, 0);

    CLK_EnableModuleClock(TMR1_MODULE);
  	CLK_SetModuleClock(TMR1_MODULE, CLK_CLKSEL1_TMR1SEL_HIRC, 0);

    CLK_SetModuleClock(SPI0_MODULE, CLK_CLKSEL2_SPI0SEL_PCLK1, MODULE_NoMsk);
    CLK_EnableModuleClock(SPI0_MODULE);

    /* Set PB multi-function pins for UART0 RXD=PB.12 and TXD=PB.13 */
    SYS->GPB_MFPH = (SYS->GPB_MFPH & ~(SYS_GPB_MFPH_PB12MFP_Msk | SYS_GPB_MFPH_PB13MFP_Msk)) |
                    (SYS_GPB_MFPH_PB12MFP_UART0_RXD | SYS_GPB_MFPH_PB13MFP_UART0_TXD);

	/* 
		SPI0 flash , 
		SPI0_FLASH_CLK : PA.2
		SPI0_FLASH_MISO : PA.1
		SPI0_FLASH_MOSI : PA.0
		SPI0_FLAH_NSS0 : PA.3
	*/
    SYS->GPA_MFPL &= ~(SYS_GPA_MFPL_PA3MFP_Msk | SYS_GPA_MFPL_PA2MFP_Msk| SYS_GPA_MFPL_PA1MFP_Msk| SYS_GPA_MFPL_PA0MFP_Msk);
    SYS->GPA_MFPL |= SYS_GPA_MFPL_PA3MFP_SPI0_SS| SYS_GPA_MFPL_PA2MFP_SPI0_CLK| SYS_GPA_MFPL_PA1MFP_SPI0_MISO| SYS_GPA_MFPL_PA0MFP_SPI0_MOSI ;


   /* Update System Core Clock */
    SystemCoreClockUpdate();

    /* Lock protected registers */
    SYS_LockReg();
}

/*
 * This is a template project for M031 series MCU. Users could based on this project to create their
 * own application without worry about the IAR/Keil project settings.
 *
 * This template application uses external crystal as HCLK source and configures UART0 to print out
 * "Hello World", users may need to do extra system configuration based on their system design.
 */

int main()
{
    SYS_Init();

	UART0_Init();
	GPIO_Init();
	TIMER1_Init();

	SpiFlash_Init();

    /* Got no where to go, just loop forever */
    while(1)
    {


    }
}

/*** (C) COPYRIGHT 2017 Nuvoton Technology Corp. ***/
