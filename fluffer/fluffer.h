/******************************************************************************
 * @file       fluffer.h
 * @version    1.0
 * @date       May 27, 2021
 * @copyright
 * @addtogroup fluffer_gp FLuffer
 * @brief	   FLuffer, a persistent buffer saved in FLASH memory.
 * @{
 *****************************************************************************/
#ifndef __FLUFFER_H__
#define __FLUFFER_H__

/**
 * @brief Error codes returned by flash memory IO handles to indicate success or failure of the operation
 **/
typedef enum fluffer_handle_error_t {
    FH_ERR_NONE,				/**<  No error occurred  */
    FH_ERR_NULLPTR,             /**<  an unexpected null pointer  */
    FH_ERR_INVALID_ADDRESS,     /**<  invalid address for read, write  */
    FH_ERR_INVALID_PAGE,        /**<  invalid page index  */
    FH_ERR_CORRUPTED_BLOCK,     /**<  corrupted page (byte read after a write operation were not the same as the bytes written)  */
}Fluffer_Handle_Error_t;

/**
 * @brief Fluffer read handle, reads data from memory
 * */
typedef Fluffer_Handle_Error_t (*Fluffer_Read_Handle_t)(uint32_t, uint8_t *, uint16_t);

/**
 * @brief Fluffer write handle, writes data to memory
 * */
typedef Fluffer_Handle_Error_t (*Fluffer_Write_Handle_t)(uint32_t, uint8_t *, uint16_t);

/**
 * @brief Fluffer erase handle, erases a memory page
 * */
typedef Fluffer_Handle_Error_t (*Fluffer_Erase_Handle_t)(uint8_t);

/**
 * @brief Fluffer handles structure, holds read, write & erase handles for the fluffer instance
 * */
typedef struct fluffer_handles_t {
    Fluffer_Read_Handle_t  read_handle;		/**<  read handle  */
    Fluffer_Write_Handle_t write_handle;    /**<  write handle  */
    Fluffer_Erase_Handle_t erase_handle;    /**<  erase handle  */
}Fluffer_Handles_t;

/**
 * @brief fluffer context, holds fluffer instance variables
 * */
typedef struct fluffer_context_t {
    uint16_t head;			/**<  head index  */
    uint16_t tail;          /**<  tail index  */
    uint16_t size;          /**<  fluffer size, maximum number of entries that can written to fluffer  */
    uint8_t  main_buffer;   /**<  main buffer block index  */
}Fluffer_Context_t;

/**
 * @brief fluffer configurations, describes memory and allocation pages for fluffer instance
 * */
typedef struct fluffer_config_t {
    uint16_t page_size;			/**<  memory page size  */
    uint8_t  word_size;	        /**<  memory word size (byte aligned = 1, half word aligned = 2, word aligned = 4)  */
    uint8_t  start_page;        /**<  allocated memory starting page index  */
    uint8_t  pages_pre_block;   /**<  allocated pages per block  */
    uint8_t  blocks;            /**<  total number of block for fluffer instance */
    uint8_t  element_size;  	/**<  fluffer element size (bytes)  */
}Fluffer_Config_t;

/**
 * @brief fluffer structure
 * */
typedef struct fluffer_t {
    Fluffer_Handles_t handles;	/**<  fluffer instance handles  */
    Fluffer_Context_t context;  /**<  fluffer instance context  */
    Fluffer_Config_t cfg;  		/**<  fluffer instance memory configurations  */
}Fluffer_t;

/**
 * @brief Fluffer reader structure, holds the index of entry to be read
 * */
typedef struct {
    uint16_t id;	/**<	index of entry to be read	*/
}Fluffer_Reader_t;

/**
 * @brief Fluffer error codes, returned by fluffer functions to indicate an error
 * */
typedef enum {
    FLUFFER_ERROR_NONE,			/**<  no errors  */
    FLUFFER_ERROR_NULLPTR,      /**<  unexpected null pointer  */
    FLUFFER_ERROR_PARAM,        /**<  unexpected parameter value  */
    //FLUFFER_ERROR_NOT_FOUND,    /**<    */
    FLUFFER_ERROR_EMPTY,        /**<  fluffer instance is empty  */
    FLUFFER_ERROR_FULL,         /**<  fluffer instance is full  */
    FLUFFER_ERROR_MEMORY,       /**<  memory access error (read, write, erase)  */
} Fluffer_Error_t;


/**
 * @brief   Initialize fluffer instance state and prepare fluffer instance for usage, depending on values in
 * 			fluffer instance configurations (cfg)
 * @details Check fluffer allocated blocks. If no blocks were found, prepares the memory for 1st time use.
 *          Otherwise, validate found blocks, and If they were corrupt, purge allocated blocks.
 * @param   psFluffer pointer to fluffer instance
 * @return  Fluffer_Error_t
 * 			FLUFFER_ERROR_NONE : if no errors occurred
 * 			FLUFFER_ERROR_NULLPTR : if psFluffer instance, or one (or more) its handles is null
 * 			FLUFFER_ERROR_PARAM : if fluffer instance configurations are invalid
 * */
Fluffer_Error_t Fluffer_enInitialize(Fluffer_t * psFluffer);

/**
 * @brief   Initialize given reader instance
 * @param   psFluffer pointer to fluffer instance
 * @param   psReader pointer to reader instance
 * @return  Fluffer_Error_t
 * 			FLUFFER_ERROR_NONE : if no errors occurred
 * 			FLUFFER_ERROR_NULLPTR : if psFluffer instance or result pointer is null
 * */
Fluffer_Error_t Fluffer_enInitReader(const Fluffer_t * const psFluffer, Fluffer_Reader_t * psReader);

/**
 * @brief   Check if fluffer instance is empty (no unmarked entries in the main buffer)
 * @param   psFluffer pointer to fluffer instance
 * @param   pu8Result pointer to a unit8_t variable, to store result in it. 1 if empty, otherwise 0
 * @return  Fluffer_Error_t
 * 			FLUFFER_ERROR_NONE : if no errors occurred
 * 			FLUFFER_ERROR_NULLPTR : if psFluffer instance, or the reader instance is null
 * */
Fluffer_Error_t Fluffer_enIsEmpty(const Fluffer_t * const psFluffer, uint8_t * pu8Result);

/**
 * @brief	Check is fluffer instance has sufficient memory to write a new entry
 * @param   psFluffer pointer to fluffer instance
 * @param   pu8Result pointer to a unit8_t variable, to store result in it. 1 if empty, otherwise 0
 * @return  Fluffer_Error_t
 * 			FLUFFER_ERROR_NONE : if no errors occurred
 * 			FLUFFER_ERROR_NULLPTR : if psFluffer instance or result pointer is null
 * */
Fluffer_Error_t Fluffer_enIsFull(const Fluffer_t * const psFluffer, uint8_t * pu8Result);

/**
 * @brief   Read entry from main buffer, pointed to by the reader instance, and copy it into given buffer
 * @param   psFluffer pointer to fluffer instance
 * @param  	psReader pointer to reader instance
 * @param	pu8Buffer pointer to buffer to copy entry into, its size must be at least @ref element_size bytes
 * @return  Fluffer_Error_t
 * 			FLUFFER_ERROR_NONE : if no errors occurred
 * 			FLUFFER_ERROR_NULLPTR : if psFluffer instance, the reader instance, or buffer pointer is null
 * */
Fluffer_Error_t Fluffer_enReadEntry(const Fluffer_t * const psFluffer, Fluffer_Reader_t * const psReader, uint8_t * const pu8Buffer);

/**
 * @brief 	Mark main buffer's head entry as pending for removal
 * @param   psFluffer pointer to fluffer instance
 * @return  Fluffer_Error_t
 * 			FLUFFER_ERROR_NONE : if no errors occurred
 * 			FLUFFER_ERROR_NULLPTR : if psFluffer instance is null
 * */
Fluffer_Error_t Fluffer_enMarkEntry(Fluffer_t * const psFluffer);

/**
 * @brief	Write given data buffer as an entry into given fluffer instance's main buffer
 * @param   psFluffer pointer to fluffer instance
 * @param	pu8Data pointer to data to be written as an entry, its size must be @ref element_size bytes
 * @return  Fluffer_Error_t
 * 			FLUFFER_ERROR_NONE : if no errors occurred
 * 			FLUFFER_ERROR_NULLPTR : if psFluffer instance, or the data pointer is null
 * */
Fluffer_Error_t Fluffer_enWriteEntry(Fluffer_t * const psFluffer, uint8_t * const pu8Data);

#endif /* __FLUFFER_H__ */

/**@}*/
