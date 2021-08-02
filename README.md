
# Fluffer

Fluffer, is a persistent FIFO buffer (FIFO queue data structure) that it is stored in permanent flash storage instead of RAM. Data stored by Fluffer will stay after reset, and all entries (buffer elements) will occupy the same space in memory.

## Table of Contents
<!-- MarkdownTOC -->

- [Motivation](#motivation)
- [Challenges](#challenges)
- [Design](#design)
    - [Software Requirements](#software-requirements)
    - [Inspiration](#inspiration)
    - [Memory Organization](#memory-organization)
    - [Read](#read)
    - [Write](#write)
    - [Clean Up](#clean-up)
    - [Migration](#migration)
- [Specs](#specs)
    - [Configuring Fluffer](#configuring-fluffer)
    - [Calculating Required Memory](#calculating-required-memory)
    - [Wear Leveling](#wear-leveling)
- [Public Types](#public-types)
    - [Fluffer_Config_t](#fluffer_config_t)
    - [Fluffer_Context_t](#fluffer_context_t)
    - [Fluffer_Handle_Error_t](#fluffer_handle_error_t)
    - [Fluffer_Handles_t](#fluffer_handles_t)
    - [Fluffer_t](#fluffer_t)
    - [Fluffer_Reader_t](#fluffer_reader_t)
    - [Fluffer_Error_t](#fluffer_error_t)
- [Public APIs](#public-apis)
    - [Fluffer_enInitialize](#fluffer_eninitialize)
    - [Fluffer_enInitReader](#fluffer_eninitreader)
    - [Fluffer_enIsEmpty](#fluffer_enisempty)
    - [Fluffer_enIsFull](#fluffer_enisfull)
    - [Fluffer_enReadEntry](#fluffer_enreadentry)
    - [Fluffer_enMarkEntry](#fluffer_enmarkentry)
    - [Fluffer_enWriteEntry](#fluffer_enwriteentry)
- [Usage](#usage)
    - [Configuration](#configuration)
    - [Example 1](#example-1)
- [Notes](#notes)

<!-- /MarkdownTOC -->


<a id="motivation"></a>
## Motivation

Fluffer was made to solve a problem that faces IoT devices. The problem is, missing telemetry data during connection blackout (internet connection is unavailable for whatever reasons). We needed a way to somehow store the telemetry data during the blackout and resend them when the connection is up again. That was challenging as the devices spent most of their time in low power mode, and the RAM contents were erased each time the device wakes up from low power mode. And we needed to store the last N unsent messages, not just the last single unsent message.
So, Fluffer was made, a persistent buffer in FLASH memory of the microcontroller. And satisfies the needs and objectives required:
1. Its persistent (stored in FLASH)
2. Stores multiple instances of the same object


<a id="challenges"></a>
## Challenges

Flash memory is a bit challenging, because it can't be overwritten and it must be erased before writing new data in it.
And there are 2 main problems with flash erase:
 - Flash memory is block erasable, a block can contain multiple pages and each page has multiple bytes.
 - Flash cells have a limited number of erase cycles, after that, the cells are corrupted and store data. And since the flash is block erasable, that translates to blocks and not individual cells. A block with some corrupted cells in it, is a considered a corrupted block and becomes unusable.

This gave rise to some challenges:
 - Limiting the number of block erase in the flash, is very important. Solution: cycle between multiple blocks to write and only erase a block before using it. This will reduce the number of erase for each block down to (1 / N). On the other hand, will use N more blocks.
 - Removing an entry from the buffer. Solution: Do not to remove entries, or to be more clear, don't erase entries. Instead add a flag to each entry, with an initial value of `0xFFFF`. When the element to be removed, overwrite the flag with a constant value. The entry becomes marked for removal.
 - Adding an entry when the buffer is full. Solution: Move unmarked entries to a secondary buffer, then add the new entry.


<a id="design"></a>
## Design

<a id="software-requirements"></a>
### Software Requirements
1. Clean and crisp API that supports basic FIFO buffer functions (initialize, push, pop, is_empty, is_full)
2. Support multiple instances to co-exist in the same memory (no overlapping), each instance has its own configurations
3. Support for on chip FLASH memory and off chip (external) FLASH memory
3. ANSI C99 compatible

<a id="inspiration"></a>
### Inspiration
Fluffer as inspired heavily by ST's application note on EEPROM emulation [^1] in FLASH memory for microcontroller that don't have an internal EEPROM.

<a id="memory-organization"></a>
### Memory Organization 

```
fluffer block:

+---------------------------+ 
|                           |  <--+ 
|                           |     |
|                           |     |
|                           |     |
|                           |     |
|                           |     |
|                           |     |
|                           |     +--- M memory pages
|                           |     |
|                           |     |
|                           |     |
|                           |     |
|                           |     |
|                           |     |
|                           |  <--+
+---------------------------+

allocated memory organization:

+----------------+----------+        +----------------+----------+        +-------------+-------------+
| Scnd Buffer #1 |          |        | Scnd Buffer #2 |          |        | Main Buffer | Entry Mark  | --> head (read)
+----------------+          |        +----------------+          |        +-------------+-------------+ 
|                           |        |                           |        |        Entry Data         |
|                           |        |                           |        +-------------+-------------+
|                           |        |                           |        | Entry Mark  | Entry Data  |
|                           |        |                           |        +-------------+-------------+
|                           |        |                           |        | Entry Data  |             | <-- tail (write)
|                           |        |                           |        +-------------+             |
|                           |        |                           |        |             .             |
|                           |        |                           |        |             .             |
|                           |        |                           |        |             .             |
|                           |        |                           |        |             .             |
|                           |        |                           |        |             .             |
|                           |        |                           |        |             .             |
|                           |        |                           |        |             .             |
+---------------------------+        +---------------------------+        +---------------------------+
```

Fluffer will allocate multiple blocks, at least 2 blocks, each block is `M` pages. A main buffer, and *at least* 1 secondary buffer. A Fluffer of 3 blocks ( 1 main buffer and 2 secondary buffers) will be used as an example throughout this document.
The main buffer is the default read/write target, and the secondary buffer is used as temporary storage during buffer clean up. Each block has a label at its first byte that defines it as a main or secondary buffer. All entries in Fluffer will have the same size.

<a id="read"></a>
### Read

Reading entries from the buffer requires a reader instance, that is initialized by fluffer function <a href="">`Fluffer_enInitReader`</a>. The reader instance points to fluffer instance's head. Reading from a reader instance returns the current entry pointed to by the reader's head. The reader is exhausted after all entries in the main buffer are read and the reader's head points to the fluffer instance's tail. This approach allows multiple independent reader for the same fluffer instance.

```text
before read:

block 0                       head                                      tail          
+-------------+-------------+-------------+-------------+-------------+-------------+
| main buffer | [M] entry 0 | [U] entry 1 | [U] entry 2 | ........... |             |
+-------------+-------------+-------------+-------------+-------------+-------------+
                             reader head

after read:

block 0                       head                                      tail          
+-------------+-------------+-------------+-------------+-------------+-------------+
| main buffer | [M] entry 0 | [U] entry 1 | [U] entry 2 | ........... |             |
+-------------+-------------+-------------+-------------+-------------+-------------+
                                           reader head

[M]         : marked entry
[U]         : unmarked entry
........... : repeat last block (entry or empty memory)
```

<a id="write"></a>
### Write

Writing entries, append them to the main buffer. When writing an entry, there are multiple scenarios:
 - main buffer has enough empty space: Fluffer will append the new entry to the main buffer.
 - main buffer is out of space: Fluffer will start clean up of main buffer, then attempt to append the new entry. There can be 2 scenarios here:
   a) There is enough space after clean up; the entry is appended to main buffer
   b) There is no space left; Fluffer will start migrating main buffer into secondary buffer

```text
before write:

block 0                       head                       tail          
+-------------+-------------+-------------+-------------+--------------+-------------+
| main buffer | [M] entry 0 | [U] entry 1 | ........... | empty memory | ........... |
+-------------+-------------+-------------+-------------+--------------+-------------+

after write:

block 0                       head                                     tail
+-------------+-------------+-------------+-------------+-------------+--------------+
| main buffer | [M] entry 0 | [U] entry 1 | ........... | [U] entry I | empty memory |
+-------------+-------------+-------------+-------------+-------------+--------------+

[M]         : marked entry
[U]         : unmarked entry
........... : repeat last block (entry or empty memory)
N           : maximum number of entries the buffer can hold, N > 0
I           : number of entries in the buffer, where 0 <= I <= N
```

<a id="clean-up"></a>
### Clean Up

```text
before clean up:

block 0                       head                                   tail
+-------------+-------------+-------------+-------------+-------------+
| main buffer | [M] entry 0 | [U] entry 1 | ........... | [U] entry N |
+-------------+-------------+-------------+-------------+-------------+

block 1
+-------------+--------------------------------------------------------+
| scnd buffer | empty memory ......................................... |
+-------------+--------------------------------------------------------+

block 2
+-------------+--------------------------------------------------------+
| scnd buffer | empty memory ......................................... |
+-------------+--------------------------------------------------------+

after clean up:

block 0
+-------------+--------------------------------------------------------+
| scnd buffer | empty memory ......................................... |
+-------------+--------------------------------------------------------+

block 1         head                                      tail
+-------------+-------------+-------------+-------------+--------------+
| main buffer | [U] entry 1 | ........... | [U] entry K | empty memory |
+-------------+-------------+-------------+-------------+--------------+

block 2
+-------------+--------------------------------------------------------+
| scnd buffer | empty memory ......................................... |
+-------------+--------------------------------------------------------+

[M]         : marked entry
[U]         : unmarked entry
........... : repeat last block (entry or empty memory)
N           : maximum number of entries the buffer can hold, N > 0
J           : number of marked entries in the buffer, 0<= J <= N
K           : number of unmarked entries in the buffer, after clean up K = N - J
```

Clean up process is how Fluffer frees space in the main buffer. Clean up is triggered when no space is left in the main buffer after an entry written. And it's the main reason the secondary buffer is required.
Fluffer cleans up the main buffer in the following steps:
 - Erase secondary buffer.
 - Copy all unmarked entries from main buffer into secondary buffer, starting from the first entry.
 - Set secondary buffer as main buffer and main buffer as secondary buffer.

<a id="migration"></a>
### Migration

```text
before clean up:

block 0         head                                                 tail
+-------------+-------------+-------------+-------------+-------------+
| main buffer | [U] entry 0 | [U] entry 1 | ........... | [U] entry N |
+-------------+-------------+-------------+-------------+-------------+

block 1
+-------------+--------------------------------------------------------+
| scnd buffer | empty memory ......................................... |
+-------------+--------------------------------------------------------+

block 2
+-------------+--------------------------------------------------------+
| scnd buffer | empty memory ......................................... |
+-------------+--------------------------------------------------------+

after clean up:

block 0
+-------------+--------------------------------------------------------+
| scnd buffer | empty memory ......................................... |
+-------------+--------------------------------------------------------+

block 1         head                                      tail
+-------------+-------------+-------------+-------------+--------------+
| main buffer | [U] entry 1 | ........... | [U] entry P | empty entry  |
+-------------+-------------+-------------+-------------+--------------+

block 2
+-------------+--------------------------------------------------------+
| scnd buffer | empty memory ......................................... |
+-------------+--------------------------------------------------------+

[M]         : marked entry
[U]         : unmarked entry
........... : repeat last block (entry or empty memory)
N           : maximum number of entries the buffer can hold, N > 0
P           : Number of entries after migration, P = N - 1
```

Migration occurs when attempting to write to a full buffer, and all entries in the main buffer are unmarked. As a result, after clean up process there will be no empty space in the new main buffer to add new entries. To solve this, Fluffer moves entries from the main buffer to the secondary buffer, except for the first entry. Then swaps the main buffer and the secondary buffers, same as clean up.

Fluffer migrates the main buffer in the following steps:
 - Erase secondary buffer.
 - Copy all unmarked entries from the main buffer into the secondary buffer, starting from the 2nd entry.
 - Set secondary buffer as main buffer and main buffer as secondary buffer.

Clean up and migration are very similar, and follow the exact same steps, except for copying entries from the main buffer into the secondary buffer. Migration can be considered as a special clean up process.

<a id="specs"></a>
## Specs

<a id="configuring-fluffer"></a>
### Configuring Fluffer

Fluffer is designed to have multiple instances on the same device. it's the users responsibility to make sure that those areas are not overlapped. 
A Fluffer instance requires the following:
1. `N`: number of blocks to allocate for Fluffer (must be > 1)
2. `M`: number of pages per block (must be > 0)
3. start page, page index at which fluffer blocks will be allocated. For example, if page index = 12, M=1, N=2, then fluffer will allocate pages 12 and 13 for as 2 separate blocks for main buffer and secondary buffer.
4. page size: a macro that defines page size in bytes
5. memory read handle: function used by fluffer instances to read bytes from the memory
6. memory write handle: function used by fluffer instances to write to memory
7. memory page erase handle: function used by fluffer instances to erase a page in memory
8. element size: number of bytes for each fluffer entry (buffer element)

<a id="calculating-required-memory"></a>
### Calculating Required Memory

Fluffer uses `N` blocks of memory, each block is `M` pages. Total memory used by a fluffer instance is `N * M * page_size` bytes, with a minimum of `(2 * page_size)` bytes (N=2 and M=1).

<a id="wear-leveling"></a>
### Wear Leveling

Fluffer distributes erase cycles between the main and secondary blocks. So, If there are `N` blocks, and each block has `K` erase cycles, each block 

<a id="public-types"></a>
## Public Types

<a id="fluffer_config_t"></a>
### Fluffer_Config_t

```C
typedef struct fluffer_config_t {
    uint16_t page_size;         /**<  memory page size  */
    uint8_t  word_size;         /**<  memory word size (byte aligned = 1, half word aligned = 2, word aligned = 4)  */
    uint8_t  start_page;        /**<  allocated memory starting page index  */
    uint8_t  pages_pre_block;   /**<  allocated pages per block  */
    uint8_t  blocks;            /**<  total number of block for fluffer instance */
    uint8_t  element_size;      /**<  fluffer element size (bytes)  */
}Fluffer_Config_t;
```
<a name="element-size"></a>

Fluffer instance configurations, describes how the fluffer instance's memory is organized and allocated.
- **page_size**: Memory page size (bytes)
- **word_size**:  Minimum number of bytes that can be written to flash memory (byte = 1, half word = 2, word = 4, double word = 8)
- **start_page**: Index of page at which fluffer instance memory will be allocated 
- **pages_pre_block**: Number of pages allocated for each fluffer block (must be > 0)
- **blocks**: Number of blocks allocated for fluffer instance ( must be > 1)
- **element_size**: Number of bytes that the fluffer element will occupy (bytes)


<a id="fluffer_context_t"></a>
### Fluffer_Context_t

```C
typedef struct fluffer_context_t {
    uint16_t head;          /**<  head index  */
    uint16_t tail;          /**<  tail index  */
    uint16_t size;          /**<  fluffer size, maximum number of entries that can written to fluffer  */
    uint8_t  main_buffer;   /**<  main buffer block index  */
}Fluffer_Context_t;
```

Fluffer context structure, holds information about current main buffer state variables.
- **head**: main buffer's head index
- **tail**: main buffer's tail index 
- **size**: main buffer size (maximum number of entries that the buffer can hold)
- **main_buffer**: index of main buffer block

<a id="fluffer_handle_error_t"></a>
### Fluffer_Handle_Error_t

```C
typedef enum fluffer_handle_error_t {
    FH_ERR_NONE,                /**<  No error occurred  */
    FH_ERR_NULLPTR,             /**<  an unexpected null pointer  */
    FH_ERR_INVALID_ADDRESS,     /**<  invalid address for read, write  */
    FH_ERR_INVALID_PAGE,        /**<  invalid page index  */
    FH_ERR_CORRUPTED_BLOCK,     /**<  corrupted page (byte read after a write operation were not the same as the bytes written)  */
}Fluffer_Handle_Error_t;
```

Error codes returned by flash memory IO handles to indicate success or failure of the operation.
Enumerations:
- **FH_ERR_NONE**: No error occurred
- **FH_ERR_NULLPTR**: an unexpected null pointer
- **FH_ERR_INVALID_ADDRESS**: invalid address for read, write
- **FH_ERR_INVALID_BLOCK**: invalid page index
- **FH_ERR_CORRUPTED_BLOCK**: corrupted page (byte read after a write operation were not the same as the bytes written)

<a id="fluffer_handles_t"></a>
### Fluffer_Handles_t

```C
typedef Fluffer_Handle_Error_t (*Fluffer_Read_Handle_t)(uint32_t, uint8_t *, uint16_t);
typedef Fluffer_Handle_Error_t (*Fluffer_Write_Handle_t)(uint32_t, uint8_t *, uint16_t);
typedef Fluffer_Handle_Error_t (*Fluffer_Erase_Handle_t)(uint8_t);

typedef struct fluffer_handles_t {
    Fluffer_Read_Handle_t  read_handle;     /**<  read handle  */
    Fluffer_Write_Handle_t write_handle;    /**<  write handle  */
    Fluffer_Erase_Handle_t erase_handle;    /**<  erase handle  */
}Fluffer_Handles_t;
```

Fluffer handles, define memory IO handles used by a fluffer instance to read, write to memory and erase memory page.
Members:
- **read_handle**: read a given number of bytes from memory into a given buffer
- **write_handle**: write a given number of bytes from a buffer into memory, fluffer will erase memory before writing to it
- **erase_handle**: erase a page with the given index (0 indexed)

<a id="fluffer_t"></a>
### Fluffer_t 

```C
typedef struct fluffer_t {
    Fluffer_Handles_t handles;  /**<  fluffer instance handles  */
    Fluffer_Context_t context;  /**<  fluffer instance context  */
    Fluffer_Config_t cfg;       /**<  fluffer instance memory configurations  */
}Fluffer_t;
```

Fluffer structure, defines a fluffer instance, its configurations, context, and FLASH memory IO handles.

<a id="fluffer_reader_t"></a>
### Fluffer_Reader_t

```C
typedef struct {
    uint16_t id;    /**<    index of entry to be read   */
}Fluffer_Reader_t;
```

Fluffer reader structure:
- **id**: index of the first unmarked entry in the main buffer (main buffer's head)

<a id="fluffer_error_t"></a>
### Fluffer_Error_t

```C
typedef enum {
    FLUFFER_ERROR_NONE,         /**<  no errors  */
    FLUFFER_ERROR_NULLPTR,      /**<  unexpected null pointer  */
    FLUFFER_ERROR_PARAM,        /**<  unexpected parameter value  */
    FLUFFER_ERROR_EMPTY,        /**<  fluffer instance is empty  */
    FLUFFER_ERROR_FULL,         /**<  fluffer instance is full  */
    FLUFFER_ERROR_MEMORY,       /**<  memory access error (read, write, erase)  */
} Fluffer_Error_t;
```

Fluffer error codes, returned by public APIs to indicate either 
success or error. Errors meaning:
- **FLUFFER_ERROR_NONE**: no errors occurred, it was a success
- **FLUFFER_ERROR_NULLPTR**: a null pointer was passed as a argument
- **FLUFFER_ERROR_PARAM**: an unexpected parameter value
- **FLUFFER_ERROR_EMPTY**: can't read, buffer is empty
- **FLUFFER_ERROR_FULL**: buffer became full after last write
- **FLUFFER_ERROR_MEMORY**: memory access error (read, write, erase)

<a id="public-apis"></a>
## Public APIs

<a id="fluffer_eninitialize"></a>
### Fluffer_enInitialize

```C
Fluffer_Error_t Fluffer_enInitialize(Fluffer_t * psFluffer)
```

Initialize fluffer instance state and prepare fluffer instance for usage, depending on values in fluffer instance configurations (cfg)

**param**
- *psFluffer* : pointer to fluffer instance 

**return**
[*Fluffer_Error_t*](#fluffer_error_t)
- *FLUFFER_ERROR_NONE* : if no errors occurred
- *FLUFFER_ERROR_NULLPTR* : if psFluffer instance, or one (or more) its handles is null
- *FLUFFER_ERROR_PARAM* : if fluffer instance configurations are invalid

<a id="fluffer_eninitreader"></a>
### Fluffer_enInitReader

```C
Fluffer_Error_t Fluffer_enInitReader(const Fluffer_t * const psFluffer, Fluffer_Reader_t * psReader)
```

Initialize given reader instance

**param**
- *psFluffer* : pointer to fluffer instance 
- *psReader* : pointer to reader instance

**return** 
[*Fluffer_Error_t*](#fluffer_error_t)
- *FLUFFER_ERROR_NONE* : if no errors occurred
- *FLUFFER_ERROR_NULLPTR* : if psFluffer instance, or the reader instance is null

<a id="fluffer_enisempty"></a>
### Fluffer_enIsEmpty
```C
Fluffer_Error_t Fluffer_enIsEmpty(const Fluffer_t * const psFluffer, uint8_t * pu8Result)
```

Check if fluffer instance is empty (no unmarked entries in the main buffer)

**param**
- *psFluffer* : pointer to fluffer instance 
- *pu8Result*: pointer to a unit8_t variable, to store result in it. 1 if empty, otherwise 0

**return** 
[*Fluffer_Error_t*](#fluffer_error_t)
- *FLUFFER_ERROR_NONE* : if no errors occurred
- *FLUFFER_ERROR_NULLPTR* : if psFluffer instance or result pointer is null

<a id="fluffer_enisfull"></a>
### Fluffer_enIsFull
```C
Fluffer_Error_t Fluffer_enIsFull(const Fluffer_t * const psFluffer, uint8_t * pu8Result)
```

Check is fluffer instance has sufficient memory to write a new entry

**param**
- *psFluffer* : pointer to fluffer instance 
- *pu8Result*: pointer to a unit8_t variable, to store result in it. 1 if empty, otherwise 0

**return** 
[*Fluffer_Error_t*](#fluffer_error_t)
- *FLUFFER_ERROR_NONE* : if no errors occurred
- *FLUFFER_ERROR_NULLPTR* : if psFluffer instance or result pointer is null

<a id="fluffer_enreadentry"></a>
### Fluffer_enReadEntry
```C
Fluffer_Error_t Fluffer_enReadEntry(const Fluffer_t * const psFluffer, Fluffer_Reader_t * const psReader, uint8_t * const pu8Buffer)
```

Read entry from main buffer, pointed to by the reader instance, and copy it into given buffer

**param**
- *psFluffer*: pointer to fluffer instance
- *psReader*: pointer to reader instance
- *pu8Buffer*: pointer to buffer to copy entry into, must be at least [element_size](#element-size) bytes

**return**
[*Fluffer_Error_t*](#fluffer_error_t)
- *FLUFFER_ERROR_NONE* : if no errors occurred
- *FLUFFER_ERROR_NULLPTR* : if psFluffer instance, the reader instance, or buffer pointer is null

<a id="fluffer_enmarkentry"></a>
### Fluffer_enMarkEntry
```C
Fluffer_Error_t Fluffer_enMarkEntry(Fluffer_t * const psFluffer)
```

Mark main buffer's head entry as pending for removal

**param**
- *psFluffer*: pointer to fluffer instance

**return**
[*Fluffer_Error_t*](#fluffer_error_t)
- *FLUFFER_ERROR_NONE* : if no errors occurred
- *FLUFFER_ERROR_NULLPTR* : if psFluffer instance, the reader instance, or buffer pointer is null

<a id="fluffer_enwriteentry"></a>
### Fluffer_enWriteEntry
```C
Fluffer_Error_t Fluffer_enWriteEntry(Fluffer_t * const psFluffer, uint8_t * const pu8Data)
```

Write given data buffer as an entry into given fluffer instance's main buffer

**param**
- *psFluffer*: pointer to fluffer instance
 *pu8Data*: pointer to data to be written as an entry, its size must be [element_size](#element-size) bytes

**return**
[*Fluffer_Error_t*](#fluffer_error_t)
- *FLUFFER_ERROR_NONE* : if no errors occurred
- *FLUFFER_ERROR_NULLPTR* : if psFluffer instance, the reader instance, or data pointer is null

<a id="usage"></a>
## Usage

<a id="configuration"></a>
### Configuration
1. in the file `fluffer_config.h`, configure the following `#define`s:
```C
/**
 * @brief Maximum number of bytes for memory word size for all fluffer
 * instances
 * */
#define FLUFFER_MAX_MEMORY_WORD_SIZE    2

/**
 * @brief Maximum number of bytes for all fluffer instances elements
 * (maximum element size for all fluffer instances)
 * */
#define FLUFFER_MAX_ELEMENT_SIZE        100

/**
 * @brief Clean byte content, shared between all fluffer instances
 * */
#define FLUFFER_CLEAN_BYTE_CONTENT      0xFF
```
  1. *FLUFFER_MAX_MEMORY_WORD_SIZE*: maximum memory word size (in bytes) for all fluffer instances. for example, if there are 3 fluffer instances, each for a different independent memory with 1, 2, 4 bytes memory words. Then this switch must be set to 4.

  2. *FLUFFER_MAX_ELEMENT_SIZE*: maximum element size (in bytes) for all fluffer instances. For example, if there are 3 fluffer instances each with element sizes 10, 20, 40. Then this switch must be set to 40.

  3. *FLUFFER_CLEAN_BYTE_CONTENT*: Content of bytes after erase, this is kinda redundant as fluffer is made specifically for flash memory. And byte contnt of flash memory after successful erase is always `0xFF`. However, make sure this is set to `0xFF`.

<a id="example-1"></a>
### Example 1

```C
#include <stdint.h>
#include <fluffer.h>

Fluffer_Handle_Error_t FlfrReadHandle(uint32_t u32Offset, uint8_t * pu8Buffer, uint16_t u16Len)
{
    // read from memory starting from address = u32Offset
    // into buffer: pu8Buffer
    // read number of bytes = u16Len
}

Fluffer_Handle_Error_t FlfrWriteHandle(uint32_t u32Offset, uint8_t * pu8Data, uint16_t u16Len)
{
    // write bytes into memory starting from address = u32Offset
    // from buffer: pu8Data
    // write number of bytes = u16Len
}

Fluffer_Handle_Error_t FlfrEraseHandle(uint8_t u8PageIndex)
{
    // erase memory page: u8pageIndex
}

int main(void)
{

    // init 
    //  .
    //  .
    //  .

    Fluffer_t Local_sFluffer;
    Fluffer_Reader_t Local_sReader;
    uint8_t Local_au8TempBuffer[100];

    //  initialize fluffer memory config
    Local_sFluffer.cfg.page_size = MEMORY_PAGE_SIZE;
    Local_sFluffer.cfg.blocks = 2;
    Local_sFluffer.cfg.pages_pre_block = 1;
    Local_sFluffer.cfg.start_page = 0;
    Local_sFluffer.cfg.word_size = 1;
    Local_sFluffer.cfg.element_size = 20;

    // set handles
    Local_sFluffer.handles.read_handle = FlfrReadHandle;
    Local_sFluffer.handles.write_handle = FlfrWriteHandle;
    Local_sFluffer.handles.erase_handle = FlfrEraseHandle;

    // initialize fluffer instance
    Fluffer_enInitialize(&Local_sFluffer);

    // initialize reader instance
    Fluffer_enInitReader(&Local_sFluffer, &Local_sReader);

    // read entry
    if(Fluffer_enReadEntry(&Local_sFluffer, &Local_sReader, Local_au8TempBuffer) == FLUFFER_ERROR_EMPTY)
    {
        // do some stuff with entry
    }
    else
    {
        // mark entry
        Fluffer_enMarkEntry(&Local_sFluffer);
    }

    // write new entry
    Fluffer_enWriteEntry(&Local_sFluffer, Local_au8TempBuffer);

    while(1)
    {

    }

    return 0;
}

```

<a id="notes"></a>
## Notes

[^1]: ST application note on [EEPROM emulation](https://www.st.com/resource/en/application_note/cd00165693-eeprom-emulation-in-stm32f10x-microcontrollers-stmicroelectronics.pdf)
