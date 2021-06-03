/******************************************************************************
 * @file      veprom_config.h
 * @brief
 * @version   1.0
 * @date      May 27, 2021
 * @copyright
 *****************************************************************************/
#ifndef __FLASH_CONFIG_H__
#define __FLASH_CONFIG_H__

#include <board_config.h>

/* ------------------------------------------------------------------------- */

/**
 * @brief Microcontroller Flash configurations
 * */
#define FLASH_START_ADDRESS		0x08000000UL
#define FLASH_TOTAL_SIZE_KB		128
#define FLASH_PAGE_COUNT		(((uint32_t)FLASH_TOTAL_SIZE_KB << 10) / FLASH_PAGE_SIZE)

/* ------------------------------------------------------------------------- */

/**
 * @brief Emulated EEPROM configurations
 * */
#define VEPROM_PAGE_SIZE		FLASH_PAGE_SIZE
#define VEPROM_START_PAGE		(FLASH_PAGE_COUNT - VEPROM_ALLOCATED_PAGES - VEPROM_PAGE_OFFSET)
#define VEPROM_ADDRESS_START	(FLASH_START_ADDRESS + (VEPROM_PAGE_SIZE * VEPROM_START_PAGE))
#define VEPROM_ADDRESS_END		(FLASH_ADDRESS_START + (VEPROM_PAGE_SIZE * VEPROM_ALLOCATED_PAGES) - 1)

#define VEPROM_OFFSET_START		0
#define VEPROM_OFFSET_END		((VEPROM_ALLOCATED_PAGES * VEPROM_PAGE_SIZE) - 1)

/* ------------------------------------------------------------------------- */

#endif /* __FLASH_CONFIG_H__ */
