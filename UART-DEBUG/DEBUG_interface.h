/*******************************************************************************
* @file      DEBUG_interface.h
* @brief
* @author    Mohammad Mohsen
* @version   1.2
* @date      Tuesday 14 Jan. 2020
* @copyright
*******************************************************************************/

#ifndef _DEBUG_INTERFACE_H_
#define _DEBUG_INTERFACE_H_


#ifdef DEBUG

#include <stdio.h>
#include <board_config.h>

#if DEBUG_UART_CHANNEL == UART_CHANNEL_1
#define DEBUG_UART_HANDLER				huart1
#elif DEBUG_UART_CHANNEL == UART_CHANNEL_2
#define DEBUG_UART_HANDLER				huart2
#elif DEBUG_UART_CHANNEL == UART_CHANNEL_3
#define DEBUG_UART_HANDLER				huart3
#endif	/*	DEBUG_UART_CHANNEL	*/

#define DebugInit()						__debug_Init()

#define Debug(fmt, ...)					printf("[%s:%d] " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#define DebugHexBuffer(buffer, len)		__debug_hex_buffer(buffer, len)

/**
 *
 **/
void __debug_Init(void);

/**
 *
 **/
void __debug_hex_buffer(uint8_t * pu8Buffer, uint32_t u32Len);

#else


#define DebugInit()
#define Debug(...)
#define DebugHexBuffer(buffer, len)

#endif	/*	 DEBUG_STATE == DEBUG_ENABLED	*/



#endif // DEBUG_INTERFACE_H_


