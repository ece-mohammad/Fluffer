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
#include <utils.h>
#include <test_fluffer.h>


#define MEMORY_PAGE_SIZE			128
#define MEMORY_PAGES				4
#define MEMORY_WORD_SIZE			2
#define BASIC_TEST_ELEMENT_SIZE		40

#define ENTRY_0						1
#define ENTRY_1						2
#define ENTRY_2						3
#define ENTRY_3						4
#define ENTRY_4						5
#define ENTRY_5						6
#define ENTRY_6						7
#define ENTRY_7						8
#define ENTRY_8						9

#define ENTRY_ID_TO_MARK_ADDRESS(fluffer, id)	((((fluffer).cfg.element_size + (fluffer).cfg.word_size) * (id)) + (fluffer).cfg.word_size)
#define ENTRY_ID_TO_ADDRESS(fluffer, id)		(ENTRY_ID_TO_MARK_ADDRESS(fluffer, id) + (fluffer).cfg.word_size)


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
 * page size = memory page size
 * blocks = 2
 * pages per block = 1
 * start page = 0
 * word size = 2
 * element size = 20
 * */
static void memcfg(Fluffer_t * psFluffer)
{
    /*	initialize fluffer memory config	*/
    psFluffer->cfg.page_size = MEMORY_PAGE_SIZE;
    psFluffer->cfg.blocks = 2;
    psFluffer->cfg.pages_pre_block = 1;
    psFluffer->cfg.start_page = 0;
    psFluffer->cfg.word_size = 2;
    psFluffer->cfg.element_size = BASIC_TEST_ELEMENT_SIZE;
}

static void fill_buffer(uint8_t * pu8Buffer, uint16_t u16Len, uint8_t u8Fill)
{
    while(u16Len--)
    {
        *pu8Buffer++ = u8Fill;
    }
}

static void test_fluffer_memory_config(void);

/**
 * Test scenario:
 * 01. initialize fluffer instance
 * 02. test is empty == true
 * 03. test is full == false
 * 04. test mark when empty
 * 05. initialize reader instance
 * 06. test read
 * 07. add entry 0
 * 08. test is empty == false
 * 09. test is full == false
 * 10. read entry & test read entry == entry 0
 * 11. test read entry == error empty
 * 12. add entry 1
 * 13. test is empty == false
 * 14. test is full == true
 * 15. read all entries (entry 0 7 entry 1)
 * 16. mark an entry (entry 0)
 * 17. init reader & test read entry == entry 1
 * 18. add 1 more entry (entry 2) & test clean up is correct (block 2 is main, has entry 1 & 2)
 * 19. test is full == false
 * 20. test is empty == false
 * 21. add 1 entry (entry 3)
 * 22. test migration is correct
 * 23. test is full == false
 * 24. test is empty == false
 * 25. read all entries
 * 26. mark all entries
 * 27. test mark entry == error empty
 * 28. add entry (entry 4)
 * 29. test clean up
 * 30. create new fluffer instance with the same configurations and initialize it
 * 31. check new instance context == old instance context
 * */
static void test_fluffer_memory_config(void)
{
    Debug("---------------- Start: %s-------------------\n", __FUNCTION__);

    uint8_t Local_au8DataBuffer[BASIC_TEST_ELEMENT_SIZE];
    Fluffer_t Local_sFluffer;
    Fluffer_t Local_sNewFluffer;
    Fluffer_Reader_t Local_sReader;
    Fluffer_Error_t Local_enError;
    uint8_t Local_u8Result;
    uint16_t Local_u16Address;

    /*	00. configure fluffer instance	*/
    memcfg(&Local_sFluffer);
    set_default_handles(&Local_sFluffer);

    /*	01. initialize instance	*/
    Debug("Test 01\n");
    Local_enError = Fluffer_enInitialize(&Local_sFluffer);
    TEST_ASSERT_EQUAL_MESSAGE(Local_enError, FLUFFER_ERROR_NONE, "Fluffer init error\n");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(MEMORY[0][0], 0, "FlufferInit failed\n");

    /*	02. test isEmpty	*/
    Debug("Test 02\n");
    Local_enError = Fluffer_enIsEmpty(&Local_sFluffer, &Local_u8Result);
    TEST_ASSERT_EQUAL_MESSAGE(Local_enError, FLUFFER_ERROR_NONE, "IsEmpty error");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(Local_u8Result, 1, "IsEmpty failed\n");

    /*	03. test isFull		*/
    Debug("Test 03\n");
    Local_enError = Fluffer_enIsFull(&Local_sFluffer, &Local_u8Result);
    TEST_ASSERT_EQUAL_MESSAGE(Local_enError, FLUFFER_ERROR_NONE, "IsFull error\n");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(Local_u8Result, 0, "IsFull failed\n");

    /*	04. test entry mark when empty	*/
    Debug("Test 04\n");
    Local_enError = Fluffer_enMarkEntry(&Local_sFluffer);
    Local_u16Address = ENTRY_ID_TO_MARK_ADDRESS(Local_sFluffer, 0);
    TEST_ASSERT_EQUAL_MESSAGE(Local_enError, FLUFFER_ERROR_EMPTY, "FlufferMark error\n");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(MEMORY[0][Local_u16Address], 0xFF, "Mark when empty failed\n");

    /*	05. initialize reader instance	*/
    Debug("Test 05\n");
    Local_enError = Fluffer_enInitReader(&Local_sFluffer, &Local_sReader);
    TEST_ASSERT_EQUAL_MESSAGE(Local_enError, FLUFFER_ERROR_NONE, "ReaderInit error\n");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(Local_sFluffer.context.head, Local_sReader.id, "ReadInit failed\n");

    /*	06. test read when empty	*/
    Debug("Test 06\n");
    memset(Local_au8DataBuffer, 0x00, sizeof(Local_au8DataBuffer));
    Local_enError = Fluffer_enReadEntry(&Local_sFluffer, &Local_sReader, Local_au8DataBuffer);
    TEST_ASSERT_EQUAL_MESSAGE(Local_enError, FLUFFER_ERROR_EMPTY, "FluffferRead error\n");

    /*	07. test write entry 0 */
    Debug("Test 07\n");
    Local_u16Address = ENTRY_ID_TO_ADDRESS(Local_sFluffer, 0);
    fill_buffer(Local_au8DataBuffer, sizeof(Local_au8DataBuffer), ENTRY_0);
    Local_enError = Fluffer_enWriteEntry(&Local_sFluffer, Local_au8DataBuffer);
    TEST_ASSERT_EQUAL_MESSAGE(Local_enError, FLUFFER_ERROR_NONE, "FlufferWrite error\n");
    TEST_ASSERT_EACH_EQUAL_UINT8_MESSAGE(ENTRY_0, &MEMORY[0][Local_u16Address], sizeof(Local_au8DataBuffer), "FlufferWrite failed\n");

    /*	08. test isFull		*/
    Debug("Test 08\n");
    Local_enError = Fluffer_enIsFull(&Local_sFluffer, &Local_u8Result);
    TEST_ASSERT_EQUAL_MESSAGE(Local_enError, FLUFFER_ERROR_NONE, "IsFull error\n");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(Local_u8Result, 0, "IsFull failed\n");

    /*	09. test isEmpty	*/
    Debug("Test 09\n");
    Local_enError = Fluffer_enIsEmpty(&Local_sFluffer, &Local_u8Result);
    TEST_ASSERT_EQUAL_MESSAGE(Local_enError, FLUFFER_ERROR_NONE, "IsEmpty error");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(Local_u8Result, 0, "IsEmpty failed\n");

    /*	10. test read entry 1	*/
    Debug("Test 10\n");
    memset(Local_au8DataBuffer, 0x00, sizeof(Local_au8DataBuffer));
    Local_enError = Fluffer_enReadEntry(&Local_sFluffer, &Local_sReader, Local_au8DataBuffer);
    TEST_ASSERT_EQUAL_MESSAGE(Local_enError, FLUFFER_ERROR_NONE, "ReadEntry error\n");
    TEST_ASSERT_EACH_EQUAL_UINT8_MESSAGE(ENTRY_0, Local_au8DataBuffer, sizeof(Local_au8DataBuffer), "ReadEntry failed\n");

    /*	11. test when all entries were read	*/
    Debug("Test 11\n");
    memset(Local_au8DataBuffer, 0x00, sizeof(Local_au8DataBuffer));
    Local_enError = Fluffer_enReadEntry(&Local_sFluffer, &Local_sReader, Local_au8DataBuffer);
    TEST_ASSERT_EQUAL_MESSAGE(Local_enError, FLUFFER_ERROR_EMPTY, "ReadEntry when empty error\n");
    TEST_ASSERT_EACH_EQUAL_UINT8_MESSAGE(0, Local_au8DataBuffer, sizeof(Local_au8DataBuffer), "ReadEntry when empty failed\n");

    /*	12. add entries until fluffer is almost full (entry 1)	*/
    Debug("Test 12\n");
    // entry 1
    fill_buffer(Local_au8DataBuffer, sizeof(Local_au8DataBuffer), ENTRY_1);
    Local_u16Address = ENTRY_ID_TO_ADDRESS(Local_sFluffer, 1);
    Local_enError = Fluffer_enWriteEntry(&Local_sFluffer, Local_au8DataBuffer);
    TEST_ASSERT_EQUAL_MESSAGE(Local_enError, FLUFFER_ERROR_NONE, "FlufferWrite entry 2 error\n");
    TEST_ASSERT_EACH_EQUAL_UINT8_MESSAGE(ENTRY_1, &MEMORY[0][Local_u16Address], sizeof(Local_au8DataBuffer), "FlufferWrite entry 2 failed\n");

#if 0
    // entry 2
    fill_buffer(Local_au8DataBuffer, sizeof(Local_au8DataBuffer), ENTRY_2);
    Local_u16Address = ENTRY_ID_TO_ADDRESS(Local_sFluffer, 2);
    Local_enError = Fluffer_enWriteEntry(&Local_sFluffer, Local_au8DataBuffer);
    TEST_ASSERT_EQUAL_MESSAGE(Local_enError, FLUFFER_ERROR_NONE, "FlufferWrite entry 3 error\n");
    TEST_ASSERT_EACH_EQUAL_UINT8_MESSAGE(ENTRY_2, &MEMORY[0][Local_u16Address], sizeof(Local_au8DataBuffer), "FlufferWrite entry 3 failed\n");
#endif

    /*	13. test isFull		*/
    Debug("Test 13\n");
    Local_enError = Fluffer_enIsFull(&Local_sFluffer, &Local_u8Result);
    TEST_ASSERT_EQUAL_MESSAGE(Local_enError, FLUFFER_ERROR_NONE, "IsFull error\n");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(Local_u8Result, 0, "IsFull failed\n");

    /*	14. test isEmpty	*/
    Debug("Test 14\n");
    Local_enError = Fluffer_enIsEmpty(&Local_sFluffer, &Local_u8Result);
    TEST_ASSERT_EQUAL_MESSAGE(Local_enError, FLUFFER_ERROR_NONE, "IsEmpty error");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(Local_u8Result, 0, "IsEmpty failed\n");

    /*	15. test read all entries	*/
    Debug("Test 15\n");
    Local_enError = Fluffer_enInitReader(&Local_sFluffer, &Local_sReader);
    TEST_ASSERT_EQUAL_MESSAGE(Local_enError, FLUFFER_ERROR_NONE, "ReaderInit error\n");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(Local_sFluffer.context.head, Local_sReader.id, "ReadInit failed\n");

    memset(Local_au8DataBuffer, 0x00, sizeof(Local_au8DataBuffer));
    Local_enError = Fluffer_enReadEntry(&Local_sFluffer, &Local_sReader, Local_au8DataBuffer);
    TEST_ASSERT_EQUAL_MESSAGE(Local_enError, FLUFFER_ERROR_NONE, "ReadEntry error\n");
    TEST_ASSERT_EACH_EQUAL_UINT8_MESSAGE(ENTRY_0, Local_au8DataBuffer, sizeof(Local_au8DataBuffer), "ReadEntry failed\n");

    memset(Local_au8DataBuffer, 0x00, sizeof(Local_au8DataBuffer));
    Local_enError = Fluffer_enReadEntry(&Local_sFluffer, &Local_sReader, Local_au8DataBuffer);
    TEST_ASSERT_EQUAL_MESSAGE(Local_enError, FLUFFER_ERROR_NONE, "ReadEntry error\n");
    TEST_ASSERT_EACH_EQUAL_UINT8_MESSAGE(ENTRY_1, Local_au8DataBuffer, sizeof(Local_au8DataBuffer), "ReadEntry failed\n");

    memset(Local_au8DataBuffer, 0x00, sizeof(Local_au8DataBuffer));
    Local_enError = Fluffer_enReadEntry(&Local_sFluffer, &Local_sReader, Local_au8DataBuffer);
    TEST_ASSERT_EQUAL_MESSAGE(Local_enError, FLUFFER_ERROR_EMPTY, "ReadEntry error\n");

    /*	16. mark an entry	*/
    Debug("Test 16\n");
    Local_enError = Fluffer_enMarkEntry(&Local_sFluffer);
    TEST_ASSERT_EQUAL_MESSAGE(Local_enError, FLUFFER_ERROR_NONE, "MarkEntry error\n");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0x00, MEMORY[0][ENTRY_ID_TO_MARK_ADDRESS(Local_sFluffer, 0)], "MarkEntry Failed\n");

    /*	17. read entry and check entry == entry 1 (not entry 0)	*/
    Debug("Test 17\n");
    Local_enError = Fluffer_enInitReader(&Local_sFluffer, &Local_sReader);
    TEST_ASSERT_EQUAL_MESSAGE(Local_enError, FLUFFER_ERROR_NONE, "InitReader error\n");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0x01, Local_sReader.id, "InitReader Failed\n");

    memset(Local_au8DataBuffer, 0x00, sizeof(Local_au8DataBuffer));
    Local_enError = Fluffer_enReadEntry(&Local_sFluffer, &Local_sReader, Local_au8DataBuffer);
    TEST_ASSERT_EQUAL_MESSAGE(Local_enError, FLUFFER_ERROR_NONE, "ReadEntry error\n");
    TEST_ASSERT_EACH_EQUAL_UINT8_MESSAGE(ENTRY_1, Local_au8DataBuffer, sizeof(Local_au8DataBuffer), "ReadEntry failed\n");

    /*	18. add entry 2	and check clean up is correct (only 1 entry in block 2)	*/
    Debug("Test 18\n");
    fill_buffer(Local_au8DataBuffer, sizeof(Local_au8DataBuffer), ENTRY_2);
    Local_u16Address = ENTRY_ID_TO_ADDRESS(Local_sFluffer, 2);
    Local_enError = Fluffer_enWriteEntry(&Local_sFluffer, Local_au8DataBuffer);
    TEST_ASSERT_EQUAL_MESSAGE(Local_enError, FLUFFER_ERROR_NONE, "FlufferWrite error\n");
    // clean up test
    Local_u16Address = ENTRY_ID_TO_ADDRESS(Local_sFluffer, 0);
    TEST_ASSERT_EACH_EQUAL_UINT8_MESSAGE(ENTRY_1, &MEMORY[1][Local_u16Address], sizeof(Local_au8DataBuffer), "CleanUp failed @entry 1\n");
    Local_u16Address = ENTRY_ID_TO_ADDRESS(Local_sFluffer, 1);
    TEST_ASSERT_EACH_EQUAL_UINT8_MESSAGE(ENTRY_2, &MEMORY[1][Local_u16Address], sizeof(Local_au8DataBuffer), "CleanUp failed @entry 2\n");
    Local_u16Address = ENTRY_ID_TO_ADDRESS(Local_sFluffer, 2);
    TEST_ASSERT_EACH_EQUAL_UINT8_MESSAGE(0xFF, &MEMORY[1][Local_u16Address - 1], sizeof(Local_au8DataBuffer) + 1, "CleanUp failed @entry 3\n");
    TEST_ASSERT_EQUAL_INT16_MESSAGE(0, Local_sFluffer.context.head, "CleanUp failed @head\n");
    TEST_ASSERT_EQUAL_INT16_MESSAGE(2, Local_sFluffer.context.tail, "CleanUp failed @tail\n");

    /*	19. test is full == false	*/
    Debug("Test 19\n");
    Local_enError = Fluffer_enIsFull(&Local_sFluffer, &Local_u8Result);
    TEST_ASSERT_EQUAL_MESSAGE(Local_enError, FLUFFER_ERROR_NONE, "isFull error\n");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(Local_u8Result, FALSE, "IsFull failed\n");

    /*	20. test is empty == false	*/
    Debug("Test 20\n");
    Local_enError = Fluffer_enIsEmpty(&Local_sFluffer, &Local_u8Result);
    TEST_ASSERT_EQUAL_MESSAGE(Local_enError, FLUFFER_ERROR_NONE, "IsEmpty error\n");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(Local_u8Result, FALSE, "IsEmpty failed\n");

    /*	21. add entry 3	*/
    Debug("Test 21\n");
    fill_buffer(Local_au8DataBuffer, sizeof(Local_au8DataBuffer), ENTRY_3);
    Local_u16Address = ENTRY_ID_TO_ADDRESS(Local_sFluffer, 2);
    Local_enError = Fluffer_enWriteEntry(&Local_sFluffer, Local_au8DataBuffer);
    TEST_ASSERT_EQUAL_MESSAGE(Local_enError, FLUFFER_ERROR_NONE, "FlufferWrite error\n");

    /*	22. test migration	*/
    Debug("Test 21\n");
    Local_u16Address = ENTRY_ID_TO_ADDRESS(Local_sFluffer, 0);
    TEST_ASSERT_EACH_EQUAL_UINT8_MESSAGE(ENTRY_2, &MEMORY[0][Local_u16Address], sizeof(Local_au8DataBuffer), "Migration failed @entry 1\n");
    Local_u16Address = ENTRY_ID_TO_ADDRESS(Local_sFluffer, 1);
    TEST_ASSERT_EACH_EQUAL_UINT8_MESSAGE(ENTRY_3, &MEMORY[0][Local_u16Address], sizeof(Local_au8DataBuffer), "Migration failed @entry 2\n");
    Local_u16Address = ENTRY_ID_TO_ADDRESS(Local_sFluffer, 2);
    TEST_ASSERT_EACH_EQUAL_UINT8_MESSAGE(0xFF, &MEMORY[1][Local_u16Address - 1], sizeof(Local_au8DataBuffer) + 1, "Migration failed @entry 3\n");
    TEST_ASSERT_EQUAL_INT16_MESSAGE(0, Local_sFluffer.context.head, "Migration failed @head\n");
    TEST_ASSERT_EQUAL_INT16_MESSAGE(2, Local_sFluffer.context.tail, "Migration failed @tail\n");

    /*	23. test is full == false	*/
    Debug("Test 23\n");
    Local_enError = Fluffer_enIsFull(&Local_sFluffer, &Local_u8Result);
    TEST_ASSERT_EQUAL_MESSAGE(Local_enError, FLUFFER_ERROR_NONE, "IFull error\n");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(Local_u8Result, FALSE, "IsFull failed\n");

    /*	24. test is empty == false	*/
    Debug("Test 24\n");
    Local_enError = Fluffer_enIsEmpty(&Local_sFluffer, &Local_u8Result);
    TEST_ASSERT_EQUAL_MESSAGE(Local_enError, FLUFFER_ERROR_NONE, "IsEmpty error\n");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(Local_u8Result, FALSE, "IsEmpty failed\n");

    /*	25. read all entries	*/
    Debug("Test 25\n");
    Local_enError = Fluffer_enInitReader(&Local_sFluffer, &Local_sReader);
    TEST_ASSERT_EQUAL_MESSAGE(Local_enError, FLUFFER_ERROR_NONE, "InitReader error\n");

    // read entry 2
    memset(Local_au8DataBuffer, 0x00, sizeof(Local_au8DataBuffer));
    Local_enError = Fluffer_enReadEntry(&Local_sFluffer, &Local_sReader, Local_au8DataBuffer);
    TEST_ASSERT_EQUAL_MESSAGE(Local_enError, FLUFFER_ERROR_NONE, "ReadEntry error\n");
    TEST_ASSERT_EACH_EQUAL_UINT8_MESSAGE(ENTRY_2, Local_au8DataBuffer, sizeof(Local_au8DataBuffer), "ReadEntry failed\n");

    // read entry 3
    memset(Local_au8DataBuffer, 0x00, sizeof(Local_au8DataBuffer));
    Local_enError = Fluffer_enReadEntry(&Local_sFluffer, &Local_sReader, Local_au8DataBuffer);
    TEST_ASSERT_EQUAL_MESSAGE(Local_enError, FLUFFER_ERROR_NONE, "ReadEntry error\n");
    TEST_ASSERT_EACH_EQUAL_UINT8_MESSAGE(ENTRY_3, Local_au8DataBuffer, sizeof(Local_au8DataBuffer), "ReadEntry failed\n");

    /*	26. mark all entries	*/
    Debug("Test 26\n");
    Local_enError = Fluffer_enMarkEntry(&Local_sFluffer);
    TEST_ASSERT_EQUAL_MESSAGE(Local_enError, FLUFFER_ERROR_NONE, "MarkEntry error\n");
    Local_u16Address = ENTRY_ID_TO_MARK_ADDRESS(Local_sFluffer, 0);
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0x00, MEMORY[0][Local_u16Address], "MarkEntry failed\n");

    Local_enError = Fluffer_enMarkEntry(&Local_sFluffer);
    Local_u16Address = ENTRY_ID_TO_MARK_ADDRESS(Local_sFluffer, 1);
    TEST_ASSERT_EQUAL_MESSAGE(Local_enError, FLUFFER_ERROR_NONE, "MarkEntry error\n");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0x00, MEMORY[0][Local_u16Address], "MarkEntry failed\n");

    /*	27. test mark entry == error empty	*/
    Debug("Test 27\n");
    Local_enError = Fluffer_enMarkEntry(&Local_sFluffer);
    TEST_ASSERT_EQUAL_MESSAGE(Local_enError, FLUFFER_ERROR_EMPTY, "MarkEntry error\n");
    Local_u16Address = ENTRY_ID_TO_MARK_ADDRESS(Local_sFluffer, 2);
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0xfF, MEMORY[0][Local_u16Address], "MarkEntry failed\n");

    /*	28. add a new entry (entry 4)	*/
    Debug("Test 28\n");
    fill_buffer(Local_au8DataBuffer, sizeof(Local_au8DataBuffer), ENTRY_4);
    Local_enError = Fluffer_enWriteEntry(&Local_sFluffer, Local_au8DataBuffer);
    TEST_ASSERT_EQUAL_MESSAGE(Local_enError, FLUFFER_ERROR_NONE, "WriteEntry error\n");

    /*	29. test clean up 	*/
    Debug("Test 29\n");
    Local_u16Address = ENTRY_ID_TO_ADDRESS(Local_sFluffer, 0);
    TEST_ASSERT_EACH_EQUAL_UINT8_MESSAGE(ENTRY_4, &MEMORY[1][Local_u16Address], sizeof(Local_au8DataBuffer), "CleanUp Failed\n");
    Local_u16Address = ENTRY_ID_TO_ADDRESS(Local_sFluffer, 1);
    TEST_ASSERT_EACH_EQUAL_UINT8_MESSAGE(0xFF, &MEMORY[1][Local_u16Address], sizeof(Local_au8DataBuffer), "CleanUp Failed\n");
    Local_u16Address = ENTRY_ID_TO_ADDRESS(Local_sFluffer, 2);
    TEST_ASSERT_EACH_EQUAL_UINT8_MESSAGE(0xFF, &MEMORY[1][Local_u16Address], sizeof(Local_au8DataBuffer), "CleanUp Failed\n");

    /*	30. create new fluffer instance using same configurations & handles and initialize it 	*/
    Debug("Test 30\n");
    memcpy(&Local_sNewFluffer, &Local_sFluffer, sizeof(Fluffer_t));
    memset(&Local_sNewFluffer.context, 0x00, sizeof(Fluffer_Context_t));

    Local_enError = Fluffer_enInitialize(&Local_sNewFluffer);
    TEST_ASSERT_EQUAL_MESSAGE(Local_enError, FLUFFER_ERROR_NONE, "Init error\n");

    /*	31. test clean up 	*/
    Debug("Test 31\n");
    TEST_ASSERT_MESSAGE(Local_sNewFluffer.context.head == Local_sFluffer.context.head, "Init Failed @head");
    TEST_ASSERT_MESSAGE(Local_sNewFluffer.context.tail == Local_sFluffer.context.tail, "Init Failed @tail");
    TEST_ASSERT_MESSAGE(Local_sNewFluffer.context.size == Local_sFluffer.context.size, "Init Failed @size");
    TEST_ASSERT_MESSAGE(Local_sNewFluffer.context.main_buffer == Local_sFluffer.context.main_buffer, "Init Failed @main_buffer");
}

void setUp(void)
{
}

void tearDown(void)
{
}

void test_fluffer(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_fluffer_memory_config);
    UNITY_END();
}
