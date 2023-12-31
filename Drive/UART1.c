#include <stdio.h>
#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_rcc.h"
#include "misc.h"
#include "UART1.h"
#include <stdarg.h>
#include "string.h"
#include "MOTORandMOVE.h"
uint8_t RX_datapacket[5] = {0};
uint8_t RX_data = 0;
uint8_t Storage_Packet[5] = {0};
uint8_t TX_datapacket[5]={0xff,0xff,0xff,0xff,0xff};
void Usart1Init(unsigned int uiBaud)
{
 	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);    

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	  
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0 ;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;		
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			
	NVIC_Init(&NVIC_InitStructure);	
	
	USART_InitStructure.USART_BaudRate = uiBaud;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No ;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART1, &USART_InitStructure); 
	USART_ITConfig(USART1, USART_IT_TXE, DISABLE); 
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
	USART_ClearFlag(USART1,USART_FLAG_TC);
	USART_Cmd(USART1, ENABLE);	
	
	
	// NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	// NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	// NVIC_InitStructure.NVIC_IRQChannelSubPriority = 7;
	// NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	// NVIC_Init(&NVIC_InitStructure);
}

void UART1_SendByte(uint8_t Byte)
{
	USART_SendData(USART1, Byte);
	while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
}

void UART1_SendArray(uint8_t *Array, uint16_t Length)
{
	uint16_t i;
	for (i = 0; i < Length; i ++)
	{
		UART1_SendByte(Array[i]);
	}
}

void UART1_SendString(char *String)
{
	uint8_t i;
	for (i = 0; String[i] != '\0'; i ++)
	{
		UART1_SendByte(String[i]);
	}
}

void UART1_send_datapacket(void)
{
	UART1_SendByte(0xff);
	UART1_SendArray(TX_datapacket,5);
	UART1_SendByte(0xfe);
}

int fputc(int ch, FILE *file)
{
	while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
	USART_SendData(USART1, ch);
	return ch;
}



void UART1_Printf(char *format, ...)
{
	char String[100];
	va_list arg;
	va_start(arg, format);
	vsprintf(String, format, arg);
	va_end(arg);
	UART1_SendString(String);
}

void TX_datapacket_assignment(uint8_t data1,uint8_t data2,uint8_t data3,uint8_t data4,uint8_t data5)
{
	TX_datapacket[0] = data1;
	TX_datapacket[1] = data2;
	TX_datapacket[2] = data3;
	TX_datapacket[3] = data4;
	TX_datapacket[4] = data5;
}


void CopeCmdData(unsigned char ucData);
void USART1_IRQHandler(void)
{
	static uint8_t i= 0;
	static uint8_t RxState = 0;
	static uint8_t pRxPacket = 0;
	if (USART_GetITStatus(USART1, USART_IT_RXNE) == SET)
	{
		uint8_t RxData = USART_ReceiveData(USART1);
		
		if (RxState == 0)
		{
			if (RxData == 0xFF)
			{
				RxState = 1;
				pRxPacket = 0;
			}
		}
		else if (RxState == 1)
		{
			RX_datapacket[pRxPacket] = RxData;
			pRxPacket ++;
			if (pRxPacket > 4)
			{
				RxState = 2;
			}
		}
		else if (RxState == 2)
		{
			if (RxData == 0xFE)
			{
				RxState = 0;
                for(i=0;i<5;i++)
				{
					Storage_Packet[i] = RX_datapacket[i];
				}
				i = 0;
				if(Storage_Packet[4]!=0xfc)
				{
					lateral_movement_lenght = Centimeter_conversion_transverse(Storage_Packet[2]);
					fine_tune_Flag = 1;
				}
			}
		}
		USART_ClearITPendingBit(USART1, USART_IT_RXNE);
	}
}
	// unsigned char ucTemp;
	// if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
	// {
	// 	ucTemp = USART_ReceiveData(USART1);
	// 	CopeCmdData(ucTemp);
	// 	USART_ClearITPendingBit(USART1, USART_IT_RXNE);
	// }
	// USART_ClearITPendingBit(USART2,USART_IT_ORE);

