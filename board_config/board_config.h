/******************************************************************************
 * @file      board_config.h
 * @brief
 * @version   1.0
 * @date      Apr 27, 2021
 * @copyright
 *****************************************************************************/
#ifndef __BOARD_CONFIG_H__
#define __BOARD_CONFIG_H__

#define UART_CHANNEL_1				0
#define UART_CHANNEL_2				1
#define UART_CHANNEL_3				2

/* ---------------------------- UART configurations --------------------------- */

#define DEBUG_UART_CHANNEL			UART_CHANNEL_1


/* ---------------------------- VEPROM configurations --------------------------- */

/**
 *
 **/
#define VEPROM_ALLOCATED_PAGES		4

/**
 *
 **/
#define VEPROM_PAGE_OFFSET			0


#endif /* __BOARD_CONFIG_H__ */
