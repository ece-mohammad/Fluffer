/******************************************************************************
 * @file      test_fluffer_basic.c
 * @brief
 * @version   1.0
 * @date      May 27, 2021
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


/*	emulated flash mmory	*/
static uint8_t MEMORY[MEMORY_PAGES][MEMORY_PAGE_SIZE];

static Fluffer_Handle_Error_t FlfrReadHandle(uint32_t u32Offset, uint8_t * pu8Buffer, uint16_t u16Len)
{
    uint8_t page_index = u32Offset / MEMORY_PAGE_SIZE;
    uint16_t byte_offset = u32Offset % MEMORY_PAGE_SIZE;

    // memcpy(pu8Data, (MEMORY + u32Offset), u16Len);
    memcpy(pu8Buffer, &MEMORY[page_index][byte_offset], u16Len);
    return FH_ERR_NONE;
}

static Fluffer_Handle_Error_t FlfrWriteHandle(uint32_t u32Offset, uint8_t * pu8Data, uint16_t u16Len)
{
    uint8_t page_index = u32Offset / MEMORY_PAGE_SIZE;
    uint16_t byte_offset = u32Offset % MEMORY_PAGE_SIZE;

    // memcpy((MEMORY + u32Offset), pu8Buffer, u16Len);
    memcpy(&MEMORY[page_index][byte_offset], pu8Data, u16Len);
    return FH_ERR_NONE;
}

static Fluffer_Handle_Error_t FlfrEraseHandle(uint8_t u8PageIndex)
{
    memset(&MEMORY[u8PageIndex], 0xFF, MEMORY_PAGE_SIZE);
    return FH_ERR_NONE;
}

static void set_default_handles(Fluffer_t * psFluffer)
{
    psFluffer->handles.read_handle = FlfrReadHandle;
    psFluffer->handles.write_handle = FlfrWriteHandle;
    psFluffer->handles.erase_handle = FlfrEraseHandle;
}

/*
 * memory config to test basic fluffer operations
 * fluffer memory config:
 * blocks = 2
 * page size = memory page size
 * pages per block = 1
 * start page = 0
 * word size = 1
 * */
static void memcfg(Fluffer_t * psFluffer)
{
    /*	initialize fluffer memory config	*/
    psFluffer->cfg.blocks = 2;
    psFluffer->cfg.page_size = MEMORY_PAGE_SIZE;
    psFluffer->cfg.pages_pre_block = 1;
    psFluffer->cfg.start_page = 0;
    psFluffer->cfg.word_size = 1;
}

static void fill_buffer(uint8_t * pu8Buffer, uint8_t u8Start, uint16_t u16Len)
{
    while(u16Len--)
    {
        *pu8Buffer++ = (uint8_t)(u16Len + u8Start);
    }
}

static void test_fluffer_basic_functions(void);

/**
 * Test scenario:
 * 1. initialize fluffer instance
 * 2. initialize reader instance
 * 3. test is empty == true
 * 4. test is full == false
 * 5. test read
 * 6. add entry 1
 * 7. test is empty == false
 * 8. test is full == false
 * 9. read entry & test read == read entry
 * 10. test read entry == error empty
 * 11. add entries until fluffer is full
 * 12. test is empty == false
 * 13. test is full == true
 * 14. read all entries
 * 15. mark an entry
 * 16. test is full == true
 * 17. add 1 more entry & test clean up is correct
 * 18. test is full == false
 * 19. test is empty == false
 * 20. add 1 entry
 * 21. test is full == true
 * 22. add 1 entry & test migration is correct
 * 23. test is full == false
 * 24. test is empty == false
 * 25. read all entries
 * 26. mark all entries
 * 27. test mark entry == empty
 * */
static void test_fluffer_basic_functions(void)
{
    Debug("---------------- Start: %s-------------------\n", __FUNCTION__);

    uint8_t Local_au8DataBuffer[4];
    Fluffer_t Local_sFluffer;
    Fluffer_Reader_t Local_sReader;
    Fluffer_Error_t Local_enError;

    memcfg(&Local_sFluffer);
    set_default_handles(&Local_sFluffer);
    Local_sFluffer.cfg.element_size = 4;

    /*	initialize instance	*/
    Local_enError = Fluffer_enInitialize(&Local_sFluffer);
    TEST_ASSERT_EQUAL(Local_enError, FLUFFER_ERROR_NONE);

    /*	test entry mark when empty	*/
    Local_enError = Fluffer_enMarkEntry(&Local_sFluffer);
    TEST_ASSERT_EQUAL(Local_enError, FLUFFER_ERROR_EMPTY);

    /*	test initialize	reader	*/
    Local_enError = Fluffer_enInitReader(&Local_sFluffer, &Local_sReader);
    TEST_ASSERT_EQUAL(Local_enError, FLUFFER_ERROR_NONE);

    /*	test read when empty	*/
    memset(Local_au8DataBuffer, 0x00, sizeof(Local_au8DataBuffer));
    Local_enError = Fluffer_enReadEntry(&Local_sFluffer, &Local_sReader, &Local_au8DataBuffer);
    TEST_ASSERT_EQUAL(Local_enError, FLUFFER_ERROR_EMPTY);

    /*	test write	*/
    fill_buffer(Local_au8DataBuffer, 0, sizeof(Local_au8DataBuffer));
    Local_enError = Fluffer_enWriteEntry(&Local_sFluffer, Local_au8DataBuffer);
    TEST_ASSERT_EQUAL(Local_enError, FLUFFER_ERROR_NONE);

    /*	test read	*/
    memset(Local_au8DataBuffer, 0x00, sizeof(Local_au8DataBuffer));
    Local_enError = Fluffer_enReadEntry(&Local_sFluffer, &Local_sReader, &Local_au8DataBuffer);
    TEST_ASSERT_EQUAL(Local_enError, FLUFFER_ERROR_NONE);

    /*	test when all entries were read	*/
    memset(Local_au8DataBuffer, 0x00, sizeof(Local_au8DataBuffer));
    Local_enError = Fluffer_enReadEntry(&Local_sFluffer, &Local_sReader, &Local_au8DataBuffer);
    TEST_ASSERT_EQUAL(Local_enError, FLUFFER_ERROR_EMPTY);

    /*	test mark entry	*/
    Local_enError = Fluffer_enMarkEntry(&Local_sFluffer);
    TEST_ASSERT_EQUAL(Local_enError, FLUFFER_ERROR_NONE);

    /*	test mark entry when all entries are marked	*/
    Local_enError = Fluffer_enMarkEntry(&Local_sFluffer);
    TEST_ASSERT_EQUAL(Local_enError, FLUFFER_ERROR_EMPTY);
}

void setUp(void)
{
}

void tearDown(void)
{
}

void test_fluffer_basic(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_fluffer_basic_functions);
    UNITY_END();
}
