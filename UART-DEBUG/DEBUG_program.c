/*******************************************************************************
* @file      DEBUG_interface.c
* @brief
* @author    Mohammad Mohsen
* @version   1.2
* @date      Tuesday 14 Jan. 2020
* @copyright
*******************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <main.h>
#include <DEBUG_interface.h>

#ifdef DEBUG

#include <usart.h>


void __debug_Init(void)
{
    HAL_DBGMCU_EnableDBGSleepMode();
    HAL_DBGMCU_EnableDBGStopMode();
    HAL_DBGMCU_EnableDBGStandbyMode();
    MX_USART1_UART_Init();
}

void __debug_hex_buffer(uint8_t * pu8Buffer, uint32_t u32Len)
{
    uint32_t Local_u32Index = 0;
    for(; Local_u32Index < u32Len; Local_u32Index++)
    {
        if((Local_u32Index & 15) == 0)
        {
            printf("\n");
        }

        printf("%02X ", pu8Buffer[Local_u32Index]);
    }

    printf("\n");
}

extern int __io_putchar(char ch)
{
    return (HAL_UART_Transmit(&DEBUG_UART_HANDLER, (uint8_t *)&ch, 1, HAL_MAX_DELAY) == HAL_OK);
}

#endif	/*	DEBUG	*/

