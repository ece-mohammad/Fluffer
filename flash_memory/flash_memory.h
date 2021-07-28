/******************************************************************************
 * @file       flash_memory.h
 * @version    1.0
 * @date       May 27, 2021
 * @copyright
 * @addtogroup flash_memory_gp Flash Memory
 * @brief
 *****************************************************************************/
#ifndef __FLASH_MEMORY_H__
#define __FLASH_MEMORY_H__

#include <flash_memory_config.h>

/**
 *
 **/
typedef enum flash_memory_error_t {
    FLASH_MEMORY_ERROR_NONE,			/**<  No error  */
    FLASH_MEMORY_ERROR_ZERO_LEN,        /**<  Unexpected zero length buffer  */
    FLASH_MEMORY_ERROR_NULLPTR,         /**<  Unexpected null pointer was given as a parameter  */
    FLASH_MEMORY_ERROR_FLASH_ERROR,     /**<  Page erase/write failed due to flash error (protection, etc)	 */
    FLASH_MEMORY_ERROR_FLASH_TIMEOUT,   /**<  Write/erase failed due to timeout	*/
    FLASH_MEMORY_ERROR_INVALID_ADDRESS,	/**<  Address is out of memory range  */
    FLASH_MEMORY_ERROR_MEM_BOUNDARY,    /**<  Read/Write will overflow outside of memory range  */
}FlashMemory_Error_t;


/**
 * @brief Erase the given flash block
 * @param u8Block  flash block index
 * @return FlashMemory_Error_t
 **/
FlashMemory_Error_t FlashMemory_enErase(uint8_t u8Block);

/**
 * @brief Read data bytes from flash, into a given buffer
 * @param u23Offset  Address to start reading from
 * @param pu8Buffer  Buffer to store read data into
 * @param u16Len     Number of bytes to read
 * @return FlashMemory_Error_t
 **/
FlashMemory_Error_t FlashMemory_enRead(uint32_t u32Offset, uint8_t * pu8Buffer, uint16_t u16Len);

/**
 * @brief Write data bytes to flash, from a given buffer. Will not erase before write.
 *        Memory must be erased before the write, or else the write will fail.
 * @param u23Offset  Address to start writing to
 * @param pu8Buffer  Buffer to write data from
 * @param u16Len     Number of bytes to write
 * @return FlashMemory_Error_t
 **/
FlashMemory_Error_t FlashMemory_enWrite(uint32_t u32Offset, const uint8_t * pu8Buffer, uint16_t u16Len);


#endif /* __FLASH_H__ */
