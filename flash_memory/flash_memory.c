/******************************************************************************
 * @file      veprom.c
 * @brief
 * @version   1.0
 * @date      May 27, 2021
 * @copyright
 *****************************************************************************/

#include <stdint.h>
#include <string.h>
#include <main.h>
#include <utils.h>
#include "flash_memory.h"


/* ------------------------------------------------------------------------- */

#define FLASH_MEMORY_GET_PAGE_ADDRESS(page)	((page) * FLASH_PAGE_SIZE)

#define FLASH_MEMORY_BLOCK_ADDRESS(b)		(FLASH_MEMORY_GET_PAGE_ADDRESS(((b) + FLASH_MEMORY_START_PAGE)))
#define FLASH_MEMORY_IS_VALID_BLOCK(b)		((b) < FLASH_MEMORY_ALLOCATED_PAGES)
#define FLASH_MEMORY_OFFSET_TO_ADDRESS(o)	((o) + FLASH_MEMORY_ADDRESS_START)

#define FLASH_MEMORY_IS_16BIT_ALIGNED(a)	(((a) & 0x01) == 0)

/* ------------------------------------------------------------------------- */

FlashMemory_Error_t FlashMemory_enErase(uint8_t u8Block)
{
    FlashMemory_Error_t Local_enError = FLASH_MEMORY_ERROR_NONE;
    uint32_t Local_u32PageError = 0;
    FLASH_EraseInitTypeDef Local_sFlashErase;
    HAL_StatusTypeDef Local_enEraseError;

    /*	chec if block index is valid	*/
    if(!FLASH_MEMORY_IS_VALID_BLOCK(u8Block))
    {
        return FLASH_MEMORY_ERROR_MEM_BOUNDARY;
    }

    Local_sFlashErase.Banks = FLASH_BANK_1;
    Local_sFlashErase.NbPages = 1;
    Local_sFlashErase.TypeErase = FLASH_TYPEERASE_PAGES;
    Local_sFlashErase.PageAddress = FLASH_MEMORY_BLOCK_ADDRESS(u8Block);

    /*	unlock the flash	*/
    HAL_FLASH_Unlock();

    /*	erase page	*/
    Local_enEraseError = HAL_FLASHEx_Erase(&Local_sFlashErase, &Local_u32PageError);

    /*	check erase error	*/
    if(Local_enEraseError == HAL_ERROR)
    {
        Local_enError = FLASH_MEMORY_ERROR_FLASH_ERROR;
    }
    else if(Local_enEraseError == HAL_TIMEOUT)
    {
        Local_enError = FLASH_MEMORY_ERROR_FLASH_TIMEOUT;
    }
    else
    {
        /*	local_enEraseError = HAL_OK	*/
    }

    /*	unlock the flash	*/
    HAL_FLASH_Lock();

    return Local_enError;
}

/* ------------------------------------------------------------------------- */

FlashMemory_Error_t FlashMemory_enRead(uint32_t u32Offset, uint8_t * pu8Buffer, uint16_t u16Len)
{
    FlashMemory_Error_t Local_enError = FLASH_MEMORY_ERROR_NONE;

    if(IS_NULLPTR(pu8Buffer))
    {
        return FLASH_MEMORY_ERROR_NULLPTR;
    }

    if(IS_ZERO(u16Len))
    {
        return FLASH_MEMORY_ERROR_ZERO_LEN;
    }

    /*	check if offset is valid	*/
    if(!IN_RANGE((u32Offset + u16Len), FLASH_MEMORY_OFFSET_START, (FLASH_MEMORY_OFFSET_END + 1)))
    {
        return FLASH_MEMORY_ERROR_MEM_BOUNDARY;
    }

    /*	copy data to buffer	*/
    memcpy(pu8Buffer, (uint8_t *)FLASH_MEMORY_OFFSET_TO_ADDRESS(u32Offset), u16Len);

    return Local_enError;
}

/* ------------------------------------------------------------------------- */

FlashMemory_Error_t FlashMemory_enWrite(uint32_t u32Offset, const uint8_t * pu8Buffer, uint16_t u16Len)
{
    FlashMemory_Error_t Local_enError = FLASH_MEMORY_ERROR_NONE;
    uint16_t Local_u16DataIndex = 0;
    HAL_StatusTypeDef Local_eFlashError;
    uint32_t Local_u32WriteAddress;
    uint8_t Local_au8TempBuffer[2];
    struct __alignment_t {
        uint8_t lead_bytes[2];
        uint8_t pad_bytes[2];
    }Local_sAlignment = {0};

    if(IS_NULLPTR(pu8Buffer))
    {
        return FLASH_MEMORY_ERROR_NULLPTR;
    }

    if(IS_ZERO(u16Len))
    {
        return FLASH_MEMORY_ERROR_ZERO_LEN;
    }

    if(!IN_RANGE((u32Offset + u16Len), FLASH_MEMORY_OFFSET_START, FLASH_MEMORY_OFFSET_END + 1))
    {
        return FLASH_MEMORY_ERROR_MEM_BOUNDARY;
    }

    HAL_FLASH_Unlock();

    /*	get write address	*/
    Local_u32WriteAddress = FLASH_MEMORY_OFFSET_TO_ADDRESS(u32Offset);

    /*	if end address is not 16 bit aligned	*/
    if(!FLASH_MEMORY_IS_16BIT_ALIGNED(Local_u32WriteAddress + u16Len))
    {
        Local_sAlignment.pad_bytes[0] = pu8Buffer[u16Len - 1];
        Local_sAlignment.pad_bytes[1] = *((uint8_t *)Local_u32WriteAddress + u16Len);

        Local_eFlashError = HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,
            (Local_u32WriteAddress + u16Len - 1),
            *((uint16_t *)Local_sAlignment.pad_bytes)
        );

        if(Local_eFlashError == HAL_ERROR)
        {
            Local_enError = FLASH_MEMORY_ERROR_FLASH_ERROR;
            goto end;
        }
        else if(Local_eFlashError == HAL_TIMEOUT)
        {
            Local_enError = FLASH_MEMORY_ERROR_FLASH_TIMEOUT;
            goto end;
        }
        else
        {
            u16Len--;
        }
    }

    /*	if start address is not 16 bit aligned	*/
    if(!FLASH_MEMORY_IS_16BIT_ALIGNED(Local_u32WriteAddress))
    {
        Local_sAlignment.lead_bytes[0] = *((uint8_t*)(Local_u32WriteAddress - 1));
        Local_sAlignment.lead_bytes[1] = pu8Buffer[Local_u16DataIndex];

        Local_eFlashError = HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,
            (Local_u32WriteAddress - 1),
            *((uint16_t *)Local_sAlignment.lead_bytes)
        );

        if(Local_eFlashError == HAL_ERROR)
        {
            Local_enError = FLASH_MEMORY_ERROR_FLASH_ERROR;
            goto end;
        }
        else if(Local_eFlashError == HAL_TIMEOUT)
        {
            Local_enError = FLASH_MEMORY_ERROR_FLASH_TIMEOUT;
            goto end;
        }
        else
        {
            Local_u32WriteAddress++;
            Local_u16DataIndex++;
            u16Len--;
        }
    }

    while(Local_u16DataIndex < u16Len)
    {
        Local_au8TempBuffer[0] = pu8Buffer[Local_u16DataIndex++];
        Local_au8TempBuffer[1] = pu8Buffer[Local_u16DataIndex++];

        Local_eFlashError = HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,
            Local_u32WriteAddress,
            *((uint16_t*)Local_au8TempBuffer)
        );

        if(Local_eFlashError == HAL_ERROR)
        {
            Local_enError = FLASH_MEMORY_ERROR_FLASH_ERROR;
            goto end;
        }
        else if(Local_eFlashError == HAL_TIMEOUT)
        {
            Local_enError = FLASH_MEMORY_ERROR_FLASH_TIMEOUT;
            goto end;
        }
        else
        {
            Local_u32WriteAddress += 2;
        }
    }

    end:
    HAL_FLASH_Lock();

    return Local_enError;
}

/* ------------------------------------------------------------------------- */
