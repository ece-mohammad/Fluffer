/******************************************************************************
 * @file       fluffer.c
 * @brief
 * @version    1.0
 * @date       May 27, 2021
 * @copyright
 * @addtogroup fluffer_gp FLuffer
 * @{
 *****************************************************************************/

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <main.h>
#include <utils.h>
#include <flash_memory.h>
#include <fluffer_config.h>
#include <fluffer.h>
#include <DEBUG_interface.h>


/* ------------------------------------------------------------------------------------ */

// NOTE
// NOTE tail after clean up & mi

/* ------------------------------------------------------------------------------------ */

#ifndef MIN
#define MIN(a, b)	(((a) < (b)) ? (a) : (b))
#endif	/*	MIN	*/

/* ------------------------------------------------------------------------------------ */
/* ---------------------------------- Private Macros ---------------------------------- */
/* ------------------------------------------------------------------------------------ */

/**
 * @brief default maximum size for memory word size (4 bytes)
 * */
#define FLUFFER_DEFAULT_MAX_WORD_SIZE	4

/**
 * @brief Index of first allocated block for any fluffer instance
 * */
#define FLUFFER_FIRST_BLOCK				0

/**
 * @brief fluffer's main buffer brand
 * */
#define FLUFFER_MAIN_BUFFER_BRAND		0x00

/**
 * @brief fluffer's entry mark, marked entries are considered used and not read again
 * */
#define FLUFFER_ENTRY_MARKED			((uint8_t)~FLUFFER_CLEAN_BYTE_CONTENT)

/**
 * @brief fluffer's unmarked entry, unmarked entries are not read
 * */
#define FLUFFER_ENTRY_UNMARKED			FLUFFER_CLEAN_BYTE_CONTENT

/* ------------------------------------------------------------------------------------ */

/**
 * @brief Get start memory address of the given fluffer instance
 * */
#define FLUFFER_START_ADDRESS(psFluffer)										((psFluffer)->cfg.start_page * (psFluffer)->cfg.page_size)

/**
 * @brief Get block size of given fluffer instance
 * */
#define FLUFFER_BLOCK_SIZE(psFluffer)											((psFluffer)->cfg.page_size * (psFluffer)->cfg.pages_pre_block)

/**
 * @brief Get total number of allocated pages for given fluffer instance
 * */
#define FLUFFER_ALLOCATED_PAGES(psFluffer)										((psFluffer)->cfg.blocks * (psFluffer)->cfg.pages_pre_block)

/**
 * @brief Get total size of allocated memory for given fluffer instance
 * */
#define FLUFFER_TOTAL_SIZE(psFluffer)											(FLUFFER_ALLOCATED_PAGES(psFluffer) * (psFluffer)->cfg.page_size)

/**
 * @brief Get absolute memory page index of a relative fluffer page index
 * */
#define FLUFFER_PAGE_INDEX(psFluffer, u16PageIndex)								((psFluffer)->cfg.start_page + (u16PageIndex))

/**
 * @brief Converts an entry ID to an offset, for the given fluffer instance
 * */
#define FLUFFER_ID_TO_OFFSET(psFluffer, u8Id)									(((u8Id) * ((psFluffer)->cfg.element_size + (psFluffer)->cfg.word_size)) + ((psFluffer)->cfg.word_size * 2))

/**
 * @brief Get fluffer's block address offset
 * */
#define FLUFFER_BLOCK_OFFSET(psFluffer, u8BlockIndex)							((u8BlockIndex) * FLUFFER_BLOCK_SIZE(psFluffer))

/**
 * @brief Get given fluffer block memory address
 * */
#define FLUFFER_BLOCK_ADDRESS(psFluffer, u8BlockIndex)							(FLUFFER_START_ADDRESS(psFluffer) + FLUFFER_BLOCK_OFFSET(psFluffer, u8BlockIndex))

/**
 * @brief get starting page index of the given block
 * */
#define FLUFFER_BLOCK_START_PAGE(psFluffer, u8Block)							((psFluffer)->cfg.start_page + ((psFluffer)->cfg.pages_pre_block * (u8Block)))

/**
 * @brief Get given fluffer block entry's memory address
 * */
#define FLUFFER_BLOCK_ENTRY_ADDRESS_BY_ID(psFluffer, u8Block, u8EntryIndex)		(FLUFFER_BLOCK_ADDRESS(psFluffer, (u8Block)) + FLUFFER_ID_TO_OFFSET(psFluffer, u8EntryIndex))

/**
 * @brief Get given block entry mark's address
 * */
#define FLUFFER_BLOCK_ENTRY_MARK_ADDRESS_BY_ID(psfluffer, u8block, u8EntryIndex) (FLUFFER_BLOCK_ENTRY_ADDRESS_BY_ID(psFluffer, u8EntryIndex) - (psFluffer)->cfg.word_size)

/**
 * @brief Get given fluffer entry's memory address
 * */
#define FLUFFER_ENTRY_ADDRESS_BY_ID(psFluffer, u8EntryIndex)					FLUFFER_BLOCK_ENTRY_ADDRESS_BY_ID(psFluffer, (psFluffer)->context.main_buffer, u8EntryIndex)

/**
 * @brief gets address of an entry's mark
 * */
#define FLUFFER_ENTRY_MARK_ADDRESS_BY_ID(psfluffer, u8EntryIndex)				(FLUFFER_ENTRY_ADDRESS_BY_ID(psFluffer, u8EntryIndex) - (psFluffer)->cfg.word_size)

/**
 * @brief Get buffer's brand address
 * */
#define FLUFFER_BRAND_ADDRESS(psFluffer, u8BlockIndex)							FLUFFER_BLOCK_ADDRESS(psFluffer, u8BlockIndex)

/* ------------------------------------------------------------------------------------ */

/**
 * @brief Get max number of entries fluffer can hold
 * */
#define FLUFFER_MAX_ENTRIES(psFluffer)								((FLUFFER_BLOCK_SIZE(psFluffer) - (psFluffer)->cfg.word_size) / ((psFluffer)->cfg.element_size + psFluffer->cfg.word_size))

/**
 * @brief check if given block is a main buffer block
 * */
#define FLUFFER_BLOCK_IS_MAIN_BUFFER(psFluffer, u8BlockIndex)		(((uint8_t*)FLUFFER_BRAND_ADDRESS(psFluffer, u8BlockIndex)) == FLUFFER_MAIN_BUFFER_BRAND)


/**
 * @brief check if fluffer instance is empty
 * */
#define FLUFFER_IS_EMPTY(psFluffer)								    ((psFluffer)->context.tail == (psFluffer)->context.head)

/**
 * @brief check if fluffer instance is full
 * */
#define FLUFFER_IS_FULL(psFluffer)								    ((psFluffer)->context.tail == (psFluffer)->context.size)

/**
 * @brief Get count of entries in the main buffer
 * */
#define FLUFFER_CURRENT_ENTRIES(psFluffer)						    ((psFluffer)->context.tail - (psFluffer)->context.head)

/**
 * @brief get next fluffer block index
 * */
#define FLUFFER_NEXT_BLOCK_ID(psFluffer)							(((psFluffer)->context.main_buffer + 1) % (psFluffer)->cfg.blocks)

/* ------------------------------------------------------------------------------------ */

/**
 * @brief Checks for null pointer for a given fluffer instance
 * */
#define FLUFFER_VALIDATE_HANDLES(psFluffer) 						(! (IS_NULLPTR((psFluffer)->handles.read_handle) || \
                                                                    IS_NULLPTR((psFluffer)->handles.write_handle) || \
                                                                    IS_NULLPTR((psFluffer)->handles.erase_handle)) )

/** checks fluffer instance configurations
 * @brief
 * */
#define FLUFFER_VALIDATE_CFG(psFluffer) 							(! (((psFluffer)->cfg.blocks < 1) || \
                                                                    IS_ZERO((psFluffer)->cfg.page_size) || \
                                                                    IS_ZERO((psFluffer)->cfg.pages_pre_block) || \
                                                                    IS_ZERO((psFluffer)->cfg.word_size) || \
                                                                    IS_ZERO((psFluffer)->cfg.element_size)))

/* ------------------------------------------------------------------------------------ */
/* -------------------------------- Private Types ------------------------------------- */
/* ------------------------------------------------------------------------------------ */

/**
 * @brief Structure used internally to wrap parameters for entries copy function,
 * describes the entries to be transfered from one block to another.
 * */
typedef struct fluffer_transfer_t {
    uint16_t size;				/**<  transfer size  */
    uint16_t src_id;            /**<  source block entry id (start from here)  */
    uint16_t dst_id;            /**<  destination block entry id (write here)  */
    uint8_t  src_block;      	/**<  source block id  */
    uint8_t  dst_block;         /**<  destination block id  */
}Fluffer_Transfer_t;


/* ------------------------------------------------------------------------------------ */
/* -------------------------------- Private APIs -------------------------------------- */
/* ------------------------------------------------------------------------------------ */

/**
 * @brief  checks if given array buffer is filled with the given preset pattern
 * @param  pu8offset
 * @param  u16Len
 * @param  u8Preset
 * @return 1 if all bytes in the specified memory region has the preset value, 0 otherwise
 * */
static uint8_t Fluffer_u8IsFilled(const uint8_t * pu8Buffer, uint16_t u16Len, uint8_t u8Preset);

/**
 * @brief  Brand fluffer instance's given block as a main buffer
 * @param  psFluffer
 * @param  u8BlockIndex
 * @return void
 * */
static void Fluffer_vidBrandBlock(const Fluffer_t * const psFluffer, uint8_t u8BlockIndex);

/**
 * @brief  Check if given entry is marked
 * @param  psFluffer
 * @param  u16entryId
 * @return 1 if entry is marked, 0 otherwise
 * */
static uint8_t Fluffer_u8EntryIsMarked(const Fluffer_t * const psFluffer, uint16_t u16EntryId);

/**
 * @brief  Check if given entry is unmarked and is empty
 * @param  psFluffer
 * @param  u16entryId
 * @return 1 if entry is unmarked & is empty, 0 otherwise
 * */
static uint8_t Fluffer_u8EntryIsEmpty(const Fluffer_t * const psFluffer, uint16_t u16EntryId);

/**
 * @brief  Find the index of the first unmarked entry in the main buffer of the given
 *         fluffer instance
 * @param  psFluffer
 * @return index of fluffer's head
 * */
static uint16_t Fluffer_u16FindHead(const Fluffer_t * const psFluffer);

/**
 * @brief  Find the first empty entry's index in the main buffer of the given fluffer instance
 * @param  psFluffer
 * @return Index of fluffer's tail
 * */
static uint16_t Fluffer_u16FindTail(const Fluffer_t * const psFluffer);

/**
 * @brief  Searches for blocks marked as main buffer, returns their count and
 *         the index of the last block.
 * @param  psFluffer
 * @param  pu8BlockIndex
 * @return Index of main buffer's block
 * */
static uint8_t Fluffer_u8GetMainBufferBlocks(const Fluffer_t * const psFluffer, uint8_t * const pu8BlockIndex);

/**
 * @brief  Check if given block is branded as a main buffer
 * @param  psFluffer
 * @param  u8BlockIndex
 * @return 1 if the block is a main buffer, 0 otherwise
 * */
static uint8_t Fluffer_u8IsMainBuffer(const Fluffer_t * const psFluffer, uint8_t u8BlockIndex);

/**
 * @brief   Prepares fluffer instance's allocated memory blocks for first time use
 * @details All allocated memory blocks are erased, the first allocated block is branded as
 *          a main buffer
 * @param   psFluffer
 * @return  void
 * */
static void Fluffer_vidPrepareFluffer(Fluffer_t * const psFluffer);

/**
 * @brief  Copies unmarked entries from source block, into the destination block starting from the given entry ID
 * @param
 * @param
 * @param
 * @param
 * @return void
 * */
static void Fluffer_vidCopyEntries(const Fluffer_t * const psFluffer, const Fluffer_Transfer_t * const psTransfer);

/**
 * @brief   Clean up fluffer instance
 * @details Copy all unmarked entries from the current main buffer to the next secondary buffer
 * 			then erase the current main buffer, finally set secondary buffer as main buffer
 * @param   psFluffer
 * @return  void
 * */
static void Fluffer_vidCleanUp(Fluffer_t * const psFluffer);

/* ------------------------------------------------------------------------------------ */

/**
 * @brief Temporary buffer to read entries into during tail search, clean up
 * and migrating entries
 *
 * @see FLUFFER_MAX_ELEMENT_SIZE
 * */
static uint8_t Fluffer_au8EntryBuffer[FLUFFER_MAX_MEMORY_WORD_SIZE + FLUFFER_MAX_ELEMENT_SIZE];

/* ------------------------------------------------------------------------------------ */

/**
 * @brief  checks if given array buffer is filled with the given preset pattern
 * @param  pu8offset
 * @param  u16Len
 * @param  u8Preset
 * @return 1 if all bytes in the specified memory region has the preset value, 0 otherwise
 * */
static uint8_t Fluffer_u8IsFilled(const uint8_t * pu8Buffer, uint16_t u16Len, uint8_t u8Preset)
{
    while(u16Len--)
    {
        if(*pu8Buffer++ != u8Preset)
        {
            return 0;
        }
        else
        {
            /*	do nothing	*/
        }
    }

    return 1;
}

/* ------------------------------------------------------------------------------------ */

/**
 * @brief  Check if given block is branded as a main buffer
 * @param  psFluffer
 * @param  u8BlockIndex
 * @return 1 if the block is a main buffer, 0 otherwise
 * */
static uint8_t Fluffer_u8IsMainBuffer(const Fluffer_t * const psFluffer, uint8_t u8BlockIndex)
{
    uint32_t Local_u32BlockBrandAddress = FLUFFER_BRAND_ADDRESS(psFluffer, u8BlockIndex);	/*	address of the fluffer instance's block brand bytes	*/

    /*	read block's brand bytes into temp buffer	 */
    psFluffer->handles.read_handle(Local_u32BlockBrandAddress, Fluffer_au8EntryBuffer, psFluffer->cfg.word_size);

    /*	check if read block's brand bytes == main buffer brand */
    return Fluffer_u8IsFilled(Fluffer_au8EntryBuffer, psFluffer->cfg.word_size, FLUFFER_MAIN_BUFFER_BRAND);
}

/* ------------------------------------------------------------------------------------ */

/**
 * @brief  Searches for blocks marked as main buffer, returns their count and
 *         the index of the last block.
 * @param  psFluffer
 * @param  pu8BlockIndex
 * @return Index of main buffer's block
 * */
static uint8_t Fluffer_u8GetMainBufferBlocks(const Fluffer_t * const psFluffer, uint8_t * const pu8BlockIndex)
{
    uint8_t Local_u8BlockIndex;									/*	fluffer instance block index	*/
    uint8_t Localu8MainBufferBlocks = 0;						/*	count found blocks branded as main buffers		*/

    /*	loop over fluffer instance blocks	*/
    for(Local_u8BlockIndex = 0; (Local_u8BlockIndex < psFluffer->cfg.blocks); Local_u8BlockIndex++)
    {
        /*	check current block is a main buffer	*/
        if(Fluffer_u8IsMainBuffer(psFluffer, Local_u8BlockIndex))
        {
            /*	increment main buffer block count	*/
            Localu8MainBufferBlocks++;

            /*	set main buffer block index	*/
            (*pu8BlockIndex) = Local_u8BlockIndex;
        }
    }

    /*	return if main buffer was found or not	*/
    return Localu8MainBufferBlocks;
}

/* ------------------------------------------------------------------------------------ */

/**
 * @brief   Prepares fluffer instance's allocated memory blocks for first time use
 * @details All allocated memory blocks are erased, the first allocated block is branded as
 *          a main buffer
 * @param   psFluffer
 * @return  void
 * */
static void Fluffer_vidPrepareFluffer(Fluffer_t * const psFluffer)
{
    const uint16_t Local_u16TotalPagesNum = FLUFFER_ALLOCATED_PAGES(psFluffer);	/*	total number of allocated pages	*/
    uint16_t Local_u16PageIndex = 0;											/*	memory page index	*/

    /*	loop over fluffer pages	*/
    for(; Local_u16PageIndex < Local_u16TotalPagesNum; Local_u16PageIndex++)
    {
        /*	erase allocated fluffer pages	*/
        psFluffer->handles.erase_handle(FLUFFER_PAGE_INDEX(psFluffer, Local_u16PageIndex));
    }

    /*	mark first fluffer block as main buffer	 */
    Fluffer_vidBrandBlock(psFluffer, FLUFFER_FIRST_BLOCK);

    /*	set first block as main buffer	*/
    psFluffer->context.main_buffer = FLUFFER_FIRST_BLOCK;
}

/* ------------------------------------------------------------------------------------ */

/**
 * @brief  Brand fluffer instance's given block as a main buffer
 * @param  psFluffer
 * @param  u8BlockIndex
 * @return void
 * */
static void Fluffer_vidBrandBlock(const Fluffer_t * const psFluffer, uint8_t u8BlockIndex)
{
    const uint8_t Local_au8MainBufferBrand [FLUFFER_DEFAULT_MAX_WORD_SIZE] = {				/*	main buffer brand */
        FLUFFER_MAIN_BUFFER_BRAND, FLUFFER_MAIN_BUFFER_BRAND,
        FLUFFER_MAIN_BUFFER_BRAND, FLUFFER_MAIN_BUFFER_BRAND,
    };

    uint32_t Local_u32BrandAddress = FLUFFER_BRAND_ADDRESS(psFluffer, u8BlockIndex);		/*	block's brand address	*/

    /*	write brand to given block	*/
    psFluffer->handles.write_handle(Local_u32BrandAddress, (uint8_t *)Local_au8MainBufferBrand, psFluffer->cfg.word_size);
}

/* ------------------------------------------------------------------------------------ */

/**
 * @brief  Check if given entry is marked
 * @param  psFluffer
 * @param  u16entryId
 * @return 1 if entry is marked, 0 otherwise
 * */
static uint8_t Fluffer_u8EntryIsMarked(const Fluffer_t * const psFluffer, uint16_t u16EntryId)
{
    uint32_t Local_u32EntryAddress = FLUFFER_ENTRY_ADDRESS_BY_ID(psFluffer, u16EntryId);	/*	entry address	*/

    /*	read entry mark	into temp buffer	*/
    psFluffer->handles.read_handle(Local_u32EntryAddress, Fluffer_au8EntryBuffer, psFluffer->cfg.word_size);

    /*	check if entry mark == entry mark	*/
    return Fluffer_u8IsFilled(Fluffer_au8EntryBuffer, psFluffer->cfg.word_size, FLUFFER_ENTRY_MARKED);
}

/* ------------------------------------------------------------------------------------ */

/**
 * @brief  Check if given entry is unmarked and is empty
 * @param  psFluffer
 * @param  u16entryId
 * @return 1 if entry is unmarked & is empty, 0 otherwise
 * */
static uint8_t Fluffer_u8EntryIsEmpty(const Fluffer_t * const psFluffer, uint16_t u16EntryId)
{
    uint32_t Local_u32ReadAddress = FLUFFER_ENTRY_ADDRESS_BY_ID(psFluffer, u16EntryId);		/*	entry's starting memory address	*/

    /*	check if entry is marked	*/
    if(Fluffer_u8EntryIsMarked(psFluffer, u16EntryId) == 1)
    {
        return 0;
    }
    else
    {
        /*	continue	*/
    }

    psFluffer->handles.read_handle(Local_u32ReadAddress, Fluffer_au8EntryBuffer, psFluffer->cfg.word_size);

    /*	check if read bytes == erased bytes	*/
    return Fluffer_u8IsFilled(Fluffer_au8EntryBuffer, psFluffer->cfg.element_size, FLUFFER_CLEAN_BYTE_CONTENT);
}

/* ------------------------------------------------------------------------------------ */

/**
 * @brief  Find the index of the first unmarked entry in the main buffer of the given
 *         fluffer instance
 * @param  psFluffer
 * @return index of fluffer's head
 * */
static uint16_t Fluffer_u16FindHead(const Fluffer_t * const psFluffer)
{
    uint16_t Local_u16EntryIndex = 0;								/*	entry index	*/
    FlagStatus Local_enHeadFound = RESET;							/*	fluffer head found flag	*/

    /*	loop over entries in fluffer instance's main buffer	*/
    while((Local_u16EntryIndex < psFluffer->context.size) && (Local_enHeadFound == RESET))
    {
        /*	check if entry is not marked	*/
        if(Fluffer_u8EntryIsMarked(psFluffer, Local_u16EntryIndex) == 0)
        {
            Local_enHeadFound = SET;
        }
        else
        {
            /*	increment entry index	*/
            Local_u16EntryIndex++;
        }
    }

    /*	save head address offset	*/
    return (Local_enHeadFound == SET) ? Local_u16EntryIndex : 0;
}

/* ------------------------------------------------------------------------------------ */

/**
 * @brief  Find the first empty entry's index in the main buffer of the given fluffer instance
 * @param  psFluffer
 * @return Index of fluffer's tail
 * */
static uint16_t Fluffer_u16FindTail(const Fluffer_t * const psFluffer)
{
    uint16_t Local_u16EntryIndex = 0;									/*	entry index	*/
    FlagStatus Local_enTailFound = RESET;								/*	fluffer tail found flag	*/

    /*	loop over entries in fluffer instance's main buffer	*/
    while((Local_u16EntryIndex < psFluffer->context.size) && (Local_enTailFound == RESET))
    {
        /*	check if entry is empty	*/
        if(Fluffer_u8EntryIsEmpty(psFluffer, Local_u16EntryIndex))
        {
            Local_enTailFound = SET;
        }
        else
        {
            Local_u16EntryIndex++;
        }
    }

    /*	save head address offset	*/
    return (Local_enTailFound == SET) ? Local_u16EntryIndex : 0;
}

/* ------------------------------------------------------------------------------------ */

/**
 * @brief  Copies unmarked entries from source block, into the destination block starting from the given entry ID
 * @param
 * @param
 * @param
 * @param
 * @return void
 * */
static void Fluffer_vidCopyEntries(const Fluffer_t * const psFluffer, const Fluffer_Transfer_t * const psTransfer)
{
    uint32_t Local_u32ReadAddress;		    				/*	source block entry's address		*/
    uint32_t Local_u32WriteAddress;		    				/*	destination block entry's address	*/
    uint16_t Local_u16ReadIndex = psTransfer->src_id;		/*	read index in source block	*/
    uint16_t Local_u16WriteIndex = psTransfer->dst_id;	    /*	write index in destination block	*/

    /*	loop over entries in the source block	*/
    for(;Local_u16ReadIndex < psTransfer->size; Local_u16ReadIndex++)
    {
        /*	get source block entry's address	*/
        Local_u32ReadAddress = FLUFFER_BLOCK_ENTRY_ADDRESS_BY_ID(psFluffer, psTransfer->src_block, Local_u16ReadIndex);

        /*	get destination block entry's address	*/
        Local_u32WriteAddress = FLUFFER_BLOCK_ENTRY_ADDRESS_BY_ID(psFluffer, psTransfer->dst_block, Local_u16WriteIndex);

        /*	read entry from source buffer into temp buffer	*/
        psFluffer->handles.read_handle(Local_u32ReadAddress, Fluffer_au8EntryBuffer, psFluffer->cfg.element_size);

        /*	write entry from temp buffer into destination block	*/
        psFluffer->handles.write_handle(Local_u32WriteAddress, Fluffer_au8EntryBuffer, psFluffer->cfg.element_size);

        /*	increment write index	*/
        Local_u16WriteIndex++;

        /*	increment read index	*/
        Local_u16ReadIndex++;
    }
}

/* ------------------------------------------------------------------------------------ */

/**
 * @brief   Clean up fluffer instance
 * @details Copy all unmarked entries from the current main buffer to the next secondary buffer
 * 			then erase the current main buffer, finally set secondary buffer as main buffer
 * @param   psFluffer
 * @return  void
 * */
static void Fluffer_vidCleanUp(Fluffer_t * const psFluffer)
{
    uint8_t Local_u8NextBlock = FLUFFER_NEXT_BLOCK_ID(psFluffer);
    uint8_t Local_u8PageIndex = 0;

    Fluffer_Transfer_t Local_sTransfer = {
        .src_block = psFluffer->context.main_buffer,
        .src_id = psFluffer->context.head,
        .dst_block = Local_u8NextBlock,
        .dst_id = 0,
        .size = FLUFFER_CURRENT_ENTRIES(psFluffer)
    };

    /*	check if current entries == buffer size	*/
    if(Local_sTransfer.size == psFluffer->context.size)
    {
        Local_sTransfer.src_id++;
    }
    else
    {
        /*	do nothing	*/
    }

    /*	copy entries from current main buffer block to the next block	*/
    Fluffer_vidCopyEntries(psFluffer, &Local_sTransfer);

    /*	set next block as main buffer	*/
    Fluffer_vidBrandBlock(psFluffer, Local_u8NextBlock);

    /*	erase old buffer pages	*/
    for(;Local_u8PageIndex < psFluffer->cfg.pages_pre_block; Local_u8PageIndex++)
    {
        psFluffer->handles.erase_handle(FLUFFER_BLOCK_START_PAGE(psFluffer, psFluffer->context.main_buffer) + Local_u8PageIndex);
    }

    /*	set main buffer	*/
    psFluffer->context.main_buffer = Local_u8NextBlock;
}

/* ------------------------------------------------------------------------------------ */
/* -------------------------------- Public APIs --------------------------------------- */
/* ------------------------------------------------------------------------------------ */

Fluffer_Error_t Fluffer_enInitialize(Fluffer_t * psFluffer)
{
    /*	check for null pointers	*/
    if(IS_NULLPTR(psFluffer) || !FLUFFER_VALIDATE_HANDLES(psFluffer))
    {
        return FLUFFER_ERROR_NULLPTR;
    }

    /*	validate fluffer memory configurations	*/
    if(!FLUFFER_VALIDATE_CFG(psFluffer))
    {
        return FLUFFER_ERROR_PARAM;
    }

    /*	check blocks for main buffer	*/
    if(Fluffer_u8GetMainBufferBlocks(psFluffer, &psFluffer->context.main_buffer) != 1)
    {
        /* if main blocks == 0: format for 1st time use
         * if main blocks >  1: memory is corrupt
         * in both cases, the memory is reformatted. In case of memory corruption,
         * old data is lost
         * */
        Fluffer_vidPrepareFluffer(psFluffer);
    }
    else
    {
        /*	do nothing	*/
    }

    /*	set fluffer size (number of entries)	*/
    psFluffer->context.size = FLUFFER_MAX_ENTRIES(psFluffer);

    /*	find head	*/
    psFluffer->context.head = Fluffer_u16FindHead(psFluffer);

    /*	find tail	*/
    psFluffer->context.tail = Fluffer_u16FindTail(psFluffer);

    return FLUFFER_ERROR_NONE;
}

/* ------------------------------------------------------------------------------------ */

Fluffer_Error_t Fluffer_enInitReader(const Fluffer_t * const psFluffer, Fluffer_Reader_t * psReader)
{
    /*	check for null pointers	*/
    if(IS_NULLPTR(psFluffer) || IS_NULLPTR(psReader))
    {
        return FLUFFER_ERROR_NULLPTR;
    }

    /*	set reader entry id to fluffer's instance head	*/
    psReader->id = psFluffer->context.head;

    return FLUFFER_ERROR_NONE;
}

/* ------------------------------------------------------------------------------------ */

Fluffer_Error_t Fluffer_enIsEmpty(const Fluffer_t * const psFluffer, uint8_t * pu8Result)
{
    /*	check for null pointers	*/
    if(IS_NULLPTR(psFluffer) || IS_NULLPTR(pu8Result))
    {
        return FLUFFER_ERROR_NULLPTR;
    }

    (*pu8Result) = FLUFFER_IS_EMPTY(psFluffer);

    return FLUFFER_ERROR_NONE;
}

/* ------------------------------------------------------------------------------------ */

Fluffer_Error_t Fluffer_enIsFull(const Fluffer_t * const psFluffer, uint8_t * pu8Result)
{
    /*	check for null pointers	*/
    if(IS_NULLPTR(psFluffer) || IS_NULLPTR(pu8Result))
    {
        return FLUFFER_ERROR_NULLPTR;
    }

    (*pu8Result) = FLUFFER_IS_FULL(psFluffer);

    return FLUFFER_ERROR_NONE;
}

/* ------------------------------------------------------------------------------------ */

Fluffer_Error_t Fluffer_enReadEntry(const Fluffer_t * const psFluffer, Fluffer_Reader_t * const psReader, uint8_t * const pu8Buffer)
{
    uint32_t Local_u32EntryAddress = FLUFFER_ENTRY_ADDRESS_BY_ID(psFluffer, psReader->id);	/*	entry's memory address	*/

    /*	check for null pointers	*/
    if(IS_NULLPTR(psFluffer) || IS_NULLPTR(psReader) || IS_NULLPTR(pu8Buffer))
    {
        return FLUFFER_ERROR_NULLPTR;
    }

    /*	check if fluffer is empty	*/
    if(FLUFFER_IS_EMPTY(psFluffer))
    {
        return FLUFFER_ERROR_EMPTY;
    }

    /*	read entry into given buffer */
    psFluffer->handles.read_handle(Local_u32EntryAddress, (uint8_t *)pu8Buffer, psFluffer->cfg.element_size);

    /*	increment reader's id	*/
    psReader->id++;

    return FLUFFER_ERROR_NONE;
}

/* ------------------------------------------------------------------------------------ */

Fluffer_Error_t Fluffer_enMarkEntry(Fluffer_t * const psFluffer)
{
    uint32_t Local_u32EntryMarkAddress = FLUFFER_ENTRY_MARK_ADDRESS_BY_ID(psFluffer, psFluffer->context.head);	/*	fluffer instance head's mark memory address	*/
    const uint8_t Local_au8TempBuffer[FLUFFER_DEFAULT_MAX_WORD_SIZE] = {										/*	entry's mark	*/
        FLUFFER_ENTRY_MARKED, FLUFFER_ENTRY_MARKED,
        FLUFFER_ENTRY_MARKED, FLUFFER_ENTRY_MARKED,
    };

    /*	check for null pointers	*/
    if(IS_NULLPTR(psFluffer))
    {
        return FLUFFER_ERROR_NULLPTR;
    }

    /*	check if fluffer is empty	*/
    if(FLUFFER_IS_EMPTY(psFluffer))
    {
        return FLUFFER_ERROR_EMPTY;
    }

    /*	write to head's mark	*/
    psFluffer->handles.write_handle(Local_u32EntryMarkAddress, (uint8_t *)Local_au8TempBuffer, psFluffer->cfg.word_size);

    /*	increment fluffer instance's head	*/
    psFluffer->context.head++;

    return FLUFFER_ERROR_NONE;
}

/* ------------------------------------------------------------------------------------ */

Fluffer_Error_t Fluffer_enWriteEntry(Fluffer_t * const psFluffer, uint8_t * const pu8Data)
{
    uint32_t Local_u32EntryAddress = FLUFFER_ENTRY_ADDRESS_BY_ID(psFluffer, psFluffer->context.tail);	/*	entry's address	*/

    /*	write entry to main buffer	*/
    psFluffer->handles.write_handle(Local_u32EntryAddress, pu8Data, psFluffer->cfg.element_size);

    /*	increment tail	*/
    psFluffer->context.tail++;

    /*	check if main buffer is full	*/
    if(FLUFFER_IS_FULL(psFluffer))
    {
        Fluffer_vidCleanUp(psFluffer);
    }
    else
    {
        /*	do nothing	*/
    }

    return FLUFFER_ERROR_NONE;
}

/* ------------------------------------------------------------------------------------ */

/**@}*/

