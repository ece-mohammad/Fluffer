/******************************************************************************
 * @file      veprom.h
 * @brief
 * @version   1.0
 * @date      May 27, 2021
 * @copyright
 *****************************************************************************/
#ifndef __FLASH_H__
#define __FLASH_H__

#include <veprom_config.h>

/**
 *
 **/
typedef enum __veprom_error_t {
    VEPROM_ERROR_NONE,					/**<  No error  */
    VEPROM_ERROR_ZERO_LEN,             	/**<  Unexpected zero lenght buffer  */
    VEPROM_ERROR_NULLPTR,             	/**<  Unexpected null pointer was given as a parameter  */
    VEPROM_ERROR_FLASH_ERROR,           /**<  Page erase/write failed due to flash error (protection, etc)	 */
    VEPROM_ERROR_FLASH_TIMEOUT,        	/**<  Write/erase failed due to timeout	*/
    VEPROM_ERROR_INVALID_ADDRESS,	    /**<  Address is out of memory range  */
    VEPROM_ERROR_MEM_BOUNDARY,        	/**<  Read/Write will overflow outside of memory range  */
}Veprom_Error_t;


/**
 * @brief Erase the given flash block
 * @param u8Block [in] flash block index
 * @return Veprom_Error_t [out]
 **/
Veprom_Error_t Veprom_enErase(uint8_t u8Block);

/**
 * @brief Read data bytes from flash, into a given buffer
 * @param u23Offset [in] Address to start reading from
 * @param pu8Buffer [in] Buffer to store read data into
 * @param u16Len    [in] Number of bytes to read
 * @return Veprom_Error_t
 **/
Veprom_Error_t Veprom_enRead(uint32_t u32Offset, uint8_t * pu8Buffer, uint16_t u16Len);

/**
 * @brief Write data bytes to flash, from a given buffer. Will not erase before write.
 *        Memory must be erased before the write, or else the write will fail.
 * @param u23Offset [in] Address to start writing to
 * @param pu8Buffer [in] Buffer to write data from
 * @param u16Len    [in] Number of bytes to write
 * @return Veprom_Error_t
 **/
Veprom_Error_t Veprom_enWrite(uint32_t u32Offset, const uint8_t * pu8Buffer, uint16_t u16Len);


#endif /* __FLASH_H__ */
