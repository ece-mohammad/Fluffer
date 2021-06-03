/******************************************************************************
 * @file      fluffer.h
 * @brief
 * @version   1.0
 * @date      May 27, 2021
 * @copyright
 *****************************************************************************/
#ifndef __FLUFFER_H__
#define __FLUFFER_H__

typedef enum __fluffer_error_t {
    FLUFFER_ERROR_NONE,
    FLUFFER_ERROR_NULLPTR,
    FLUFFER_ERROR_EMPTY,
    FLUFFER_ERROR_FULL,
    FLUFFER_ERROR_NOT_FOUND,
}Fluffer_Error_t;

#endif /* __FLUFFER_H__ */
