/******************************************************************************
 * @file       fluffer_config.h
 * @brief
 * @version    1.0
 * @date       Jul 7, 2021
 * @copyright
 * @addtogroup fluffer_gp FLuffer
 * @{
 *****************************************************************************/
#ifndef __FLUFFER_CONFIG_H__
#define __FLUFFER_CONFIG_H__

/**
 * @brief Maximum number of bytes for memory word size for all fluffer
 * instances
 * */
#define FLUFFER_MAX_MEMORY_WORD_SIZE	2

/**
 * @brief Maximum number of bytes for all fluffer instances elements
 * (maximum element size for all fluffer instances)
 * */
#define FLUFFER_MAX_ELEMENT_SIZE		100

/**
 * @brief Clean byte content, shared between all fluffer instances
 * */
#define FLUFFER_CLEAN_BYTE_CONTENT		0xFF

#endif /* __FLUFFER_CONFIG_H__ */

/**@}*/
