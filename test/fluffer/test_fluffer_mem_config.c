/******************************************************************************
 * @file      test_fluffer_mem_conf.c
 * @brief
 * @version   1.0
 * @date      Jul 30, 2021
 * @copyright
 *****************************************************************************/
#include <stdint.h>
#include <string.h>
#include <main.h>
#include <DEBUG_interface.h>
#include <fluffer.h>
#include <unity.h>
#include <test_fluffer.h>


#define MEMORY_PAGE_SIZE	64
#define MEMORY_PAGES		4

/*
 * memory config to test basic fluffer operations, with start page = 1
 * fluffer memory config:
 * blocks = 2
 * page size = memory page size
 * pages per block = 1
 * start page = 1
 * word size = 1
 * */
static void memcfg_2(Fluffer_t * psFluffer)
{
    /*	initialize fluffer memory config	*/
    psFluffer->cfg.blocks = 2;
    psFluffer->cfg.page_size = MEMORY_PAGE_SIZE;
    psFluffer->cfg.pages_pre_block = 1;
    psFluffer->cfg.start_page = 1;
    psFluffer->cfg.word_size = 1;
}

/*
 * memory config to test fluffer operations when word size = 2
 * fluffer memory config:
 * blocks = 2
 * page size = memory page size
 * pages per block = 1
 * start page = 0
 * word size = 2
 * */
static void memcfg_3(Fluffer_t * psFluffer)
{
    /*	initialize fluffer memory config	*/
    psFluffer->cfg.blocks = 2;
    psFluffer->cfg.page_size = MEMORY_PAGE_SIZE;
    psFluffer->cfg.pages_pre_block = 1;
    psFluffer->cfg.start_page = 0;
    psFluffer->cfg.word_size = 2;
}

/*
 * memory config to test fluffer operations when block size = 2 pages
 * fluffer memory config:
 * blocks = 2
 * page size = memory page size
 * pages per block = 2
 * start page = 0
 * word size = 1
 * */
static void memcfg_4(Fluffer_t * psFluffer)
{
    /*	initialize fluffer memory config	*/
    psFluffer->cfg.blocks = 2;
    psFluffer->cfg.page_size = MEMORY_PAGE_SIZE;
    psFluffer->cfg.pages_pre_block = 2;
    psFluffer->cfg.start_page = 0;
    psFluffer->cfg.word_size = 1;
}


void setUp(void)
{
}

void tearDown(void)
{
}

void test_fluffer_mem_config(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_fluffer_basic_functions);
    UNITY_END();
}
