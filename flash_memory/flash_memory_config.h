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
#define FLASH_MEMORY_PAGE_SIZE		FLASH_PAGE_SIZE
#define FLASH_MEMORY_START_PAGE		(FLASH_PAGE_COUNT - FLASH_MEMORY_ALLOCATED_PAGES - FLASH_MEMORY_PAGE_OFFSET)
#define FLASH_MEMORY_ADDRESS_START	(FLASH_START_ADDRESS + (FLASH_MEMORY_PAGE_SIZE * FLASH_MEMORY_START_PAGE))
#define FLASH_MEMORY_ADDRESS_END	(FLASH_ADDRESS_START + (FLASH_MEMORY_PAGE_SIZE * FLASH_MEMORY_ALLOCATED_PAGES) - 1)

#define FLASH_MEMORY_OFFSET_START	0
#define FLASH_MEMORY_OFFSET_END		((FLASH_MEMORY_ALLOCATED_PAGES * FLASH_MEMORY_PAGE_SIZE) - 1)

/* ------------------------------------------------------------------------- */

#endif /* __FLASH_CONFIG_H__ */
