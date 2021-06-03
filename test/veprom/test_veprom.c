/******************************************************************************
 * @file      test_flash.c
 * @brief
 * @version   1.0
 * @date      May 27, 2021
 * @copyright
 *****************************************************************************/

#include <stdint.h>
#include <main.h>
#include <DEBUG_interface.h>
#include <unity.h>
#include <veprom.h>
#include <test_veprom.h>


static void test_erase_error(void);
static void test_erase_no_error(void);
static void test_write_nullptr(void);
static void test_write_zero_len(void);
static void test_write_unaligned_odd_len(void);
static void test_write_unaligned_even_len(void);
static void test_write_aligned(void);
static void test_read_nullptr(void);
static void test_read_zero_len(void);
static void test_read_no_error(void);

static void test_erase_error(void)
{
    Debug("\n\n------------- Begin: %s -------------\n", __FUNCTION__);
    Veprom_Error_t Local_enError;

    Local_enError = Veprom_enErase(VEPROM_ALLOCATED_PAGES + 10);

    TEST_ASSERT(Local_enError == VEPROM_ERROR_MEM_BOUNDARY);
}

static void test_erase_no_error(void)
{
    Debug("\n\n------------- Begin: %s -------------\n", __FUNCTION__);
    Veprom_Error_t Local_enError;

    Local_enError = Veprom_enErase(0);

    TEST_ASSERT(Local_enError == VEPROM_ERROR_NONE);
}

static void test_write_nullptr(void)
{
    Debug("\n\n------------- Begin: %s -------------\n", __FUNCTION__);
    Veprom_Error_t Local_enError;

    Local_enError = Veprom_enWrite(0, NULL, 1);

    TEST_ASSERT(Local_enError == VEPROM_ERROR_NULLPTR);
}

static void test_write_zero_len(void)
{
    Debug("\n\n------------- Begin: %s -------------\n", __FUNCTION__);

    const uint8_t Local_au8TempBuffer[2] = {0xaa, 0xaa};

    Veprom_Error_t Local_enError;

    Local_enError = Veprom_enWrite(0, Local_au8TempBuffer, 0);

    TEST_ASSERT(Local_enError == VEPROM_ERROR_ZERO_LEN);
}

static void test_write_unaligned_odd_len(void)
{
    Debug("\n\n------------- Begin: %s -------------\n", __FUNCTION__);

    const uint8_t Local_au8TempBuffer[3] = {'1', '2', '3'};

    Veprom_Error_t Local_enError;

    Local_enError = Veprom_enWrite(1, Local_au8TempBuffer, sizeof(Local_au8TempBuffer));

    TEST_ASSERT(Local_enError == VEPROM_ERROR_NONE);
}

static void test_write_unaligned_even_len(void)
{
    Debug("\n\n------------- Begin: %s -------------\n", __FUNCTION__);

    const uint8_t Local_au8TempBuffer[4] = {'5', '6', '7', '8'};

    Veprom_Error_t Local_enError;

    Local_enError = Veprom_enWrite(5, Local_au8TempBuffer, sizeof(Local_au8TempBuffer));

    TEST_ASSERT(Local_enError == VEPROM_ERROR_NONE);
}

static void test_write_aligned(void)
{
    Debug("\n\n------------- Begin: %s -------------\n", __FUNCTION__);

    const uint8_t Local_au8TempBuffer[6] = {'A', 'B', 'C', 'D', 'E', 'F'};

    Veprom_Error_t Local_enError;

    Local_enError = Veprom_enWrite(10, Local_au8TempBuffer, sizeof(Local_au8TempBuffer));

    TEST_ASSERT(Local_enError == VEPROM_ERROR_NONE);
}

static void test_read_nullptr(void)
{
    Debug("\n\n------------- Begin: %s -------------\n", __FUNCTION__);

    uint8_t Local_au8TempBuffer[6] = {0};

    Veprom_Error_t Local_enError;

    Local_enError = Veprom_enRead(10, NULL, sizeof(Local_au8TempBuffer));

    TEST_ASSERT(Local_enError == VEPROM_ERROR_NULLPTR);
}

static void test_read_zero_len(void)
{
    Debug("\n\n------------- Begin: %s -------------\n", __FUNCTION__);

    uint8_t Local_au8TempBuffer[6] = {0};

    Veprom_Error_t Local_enError;

    Local_enError = Veprom_enRead(10, Local_au8TempBuffer, 0);

    TEST_ASSERT(Local_enError == VEPROM_ERROR_ZERO_LEN);
}

static void test_read_no_error(void)
{
    Debug("\n\n------------- Begin: %s -------------\n", __FUNCTION__);

    const uint8_t Local_au8Expected[6] = {'A', 'B','C', 'D', 'E', 'F'};
    uint8_t Local_au8TempBuffer[6] = {0};

    Veprom_Error_t Local_enError;

    Local_enError = Veprom_enRead(10, Local_au8TempBuffer, sizeof(Local_au8TempBuffer));

    TEST_ASSERT(Local_enError == VEPROM_ERROR_NONE);
    TEST_ASSERT_EQUAL_CHAR_ARRAY(Local_au8Expected, Local_au8TempBuffer, 6);
}


void setUp(void)
{
}

void tearDown(void)
{
}


void test_veprom(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_erase_error);
    RUN_TEST(test_erase_no_error);
    RUN_TEST(test_write_nullptr);
    RUN_TEST(test_write_zero_len);
    RUN_TEST(test_write_unaligned_odd_len);
    RUN_TEST(test_write_unaligned_even_len);
    RUN_TEST(test_write_aligned);
    RUN_TEST(test_read_nullptr);
    RUN_TEST(test_read_zero_len);
    RUN_TEST(test_read_no_error);

    UNITY_END();
}
