
# FLuffer

FLuffer, or Flash buffer, is a buffer, that it is stored in
permanent flash storage instead of RAM. 

FLuffer reads and writes data persistently in a FIFO manner.
Data stored by FLuffer will stay after reset. All entries 
(buffer elements) will occupy the same space in memory.


### Challenges

Flash storage is a bit challenging, because it can't be 
overwritten, it must be erased before writing new data in it. 
And there are 2 main problems with flash erase:

 - Flash is block erasable, and each block can hold 
   multiple entries.

 - Flash cells have a limited number of erase cycles, after 
   that, the cells are corrupted. And since the flash is
   block erasable, that translates to blocks and not 
   individual cells.

This gives rise to some challenges:

 - Limiting the number of block erase in the flash, is very
   important:
   cycle between multiple blocks to write and only erase a
   block before using it. This will reduce the number of erase
   for each block down to (1 / N). On the other hand, will
   use N more blocks.

 - Removing an entry from the buffer:
   Don't remove entries, or to be more clear, don't erase 
   entries. Instead add a flag to each entry, with an initial 
   value of `0xFFFF`. When the element to be removed, overwrite
   the flag with a different value. The entry becomes marked
   for removal.
 
 - Adding an entry when the buffer is full:
   Move unmarked entries to a secondary buffer, then add 
   the new entry.


### Design

#### Memory Organization 

```
+---------------------------+      +---------------------------+
| Scnd Buffer |             |      | Main Buffer | Entry Start |  <----- Write
+-------------+             |      +-------------+-------------+  -----> Read
|                           |      | Entry Mark  | Entry Data  |
|                           |      +---------------------------+
|                           |      |        Entry Data         |
|                           |      +---------------------------+
|                           |      | Entry Start | Entry Mark  |
|                           |      +---------------------------+
|                           |      |        Entry Data         |
|                           |      +---------------------------+
|                           |      | Entry Data  |             |
|                           |      +-------------+             |
|                           |      |             .             |
|                           |      |             .             |
|                           |      |             .             |
|                           |      |             .             |
|                           |      |             .             |
+---------------------------+      +---------------------------+

```

FLuffer will allocate 2 areas. A main buffer, and a secondary 
buffer. The main buffer is the default read/write target to
add new entries. The secondary buffer is used as temporary 
storage during buffer clean up.

Each buffer has a label at its first byte that defines 
it as a main or secondary buffer.

All entries in FLuffer have the same size in memory.

#### Read

When reading an entry, FLuffer will go to main buffer, and 
look for first unmarked entry and read it. As subsequent call
to read will get the next unmarked entry, unless the reader 
is reset.

#### Write

Writing entries, append them to the main buffer. When writing 
an entry, there are multiple scenarios:

 - main buffer has enough empty space:
   FLuffer will append the new entry to the main 
   buffer.
   
 - main buffer is out of space:
   FLuffer will start clean up of main buffer,
   then attempt to append the new entry.
   There can be 2 scenarios here:
   
   a) There is enough space after clean up:
   The entry is appended to main buffer
   
   b) There is no space left:
   FLuffer will start migrating main buffer into
   secondary buffer

#### Clean Up

Clean up process is the main reason the secondary buffer 
exists. Also, It's how FLuffer frees space in the main 
buffer.  Clean up is triggered when no space is left in 
the main buffer after an entry write.

FLuffer cleans up the main buffer in the following steps:

 - Erase secondary buffer.

 - Copy all unmarked entries from main buffer into 
   secondary buffer, starting from the first entry.
   
 - Set secondary buffer as main buffer and
 main buffer as secondary buffer.

#### Migrating

Migration occurs when attempting to write to a full buffer,
and there were no space left in the buffer after clean up.

FLuffer moves entries from the main buffer to the secondary
buffer, except for the first entry. The new entry replaces 
the it. Then swaps the main buffer and the secondary buffers,
same as clean up.

FLuffer migrates the main buffer in the following steps:

 - Erase secondary buffer.

 - Copy all unmarked entries from the main buffer into
   the secondary buffer, starting from the 2nd entry.

 - Set secondary buffer as main buffer and 
   main buffer as secondary buffer.
   
 - Write the new entry to main buffer at position 0.

Clean up and migration are very similar, and follow the
exact same steps, except for copying entries from the 
main buffer into the secondary buffer.

One last thing to talk about, is FLuffer cleanse.


#### Cleansing

Cleansing occurs when FLuffer memory is corrupted. That is when
there are 2 main buffer areas. It's an attempted recovery
process.
The buffer is corrupted when 2 areas have the main buffer 
brand. That occurs if the clean up process was interrupted
by a reset.
Under normal conditions, main buffer contains data and
secondary buffer is erased.
To restore FLuffer, the contents of the 2 areas are checked for
data. The area that contains more unchecked entries, will be 
the main buffer, the other area will be the 
secondary buffer.


### Public Types

##### Fluffer_Entry_t

```
+---------------------------------------+
| typedef struct __Fluffer_entry_t {    |
|     uint8_t data[Fluffer_ENTRY_SIZE]; |
|     uint16_t id;                      |
| } Fluffer_Entry_t;                    |
+---------------------------------------+
```

  FLuffer entry object, members:
  - data: contains the entry data, used to read the entry. 
  - id: entry's id, used internally by FLuffer to distinguish
  entries from each other.


##### Fluffer_Reader_t

```
+--------------------------------------+  
| typedef struct {                     |
|     uint32_t read_offset;            |
|   }Fluffer_Reader_t;                 |
+--------------------------------------+
```

  FLuffer reader object, members:
  - read_offset: address in the main buffer to start reading 
  from, used internally by FLuffer logic.


##### Fluffer_Error_t
```
+--------------------------------------+  
| typedef enum {                       |
|     Fluffer_ERROR_NONE,              |
|     Fluffer_ERROR_NULLPTR,           |
|     Fluffer_ERROR_NOT_FOUND,         |
|     Fluffer_ERROR_EMPTY,             |
|     Fluffer_ERROR_FULL,              |
|     Fluffer_READ_CPLT,               |
| } Fluffer_Error_t;                   |
+--------------------------------------+
```

  FLuffer error codes, returned by public APIs to indicate either 
  success or error. Errors meaning:

  - **Fluffer_ERROR_NONE**: no errors occurred, it was a success
  - **Fluffer_ERROR_NULLPTR**: a null pointer was passed as a argument
  - **Fluffer_ERROR_NOT_FOUND**: entry is not found
  - **Fluffer_ERROR_EMPTY**: can't read, buffer is empty
  - **Fluffer_ERROR_FULL**: buffer became full after last write
  - **Fluffer_READ_CPLT**: read all entries from buffer


### Public APIs

##### void Fluffer_vidInitialize(Fluffer_Handles_t * psHandles)

  Initializes FLuffer state, and handles used to read/write
  from/to memory
  
   - check both areas for main buffer brand. if no areas
     found, prepares the memory for 1st time use.
  
   - check that both area0 and area1 are not of the same type
     (one is main buffer, the other is secondary). If both are
     the same, start buffer cleanse.
  
   - If area0 is main buffer, set FLuffer's buffer address 
     offset to area0's address, else set it to area1's address
  
   - Set internal reader to main buffer at 1st unmarked entry
  
   - Set internal writer to main buffer at 1st free location


##### Fluffer_Error_t Fluffer_enInitReader(Fluffer_Reader_t * psReader)

  Initializes an instance of FLuffer reader
  
   - Set reader object's `read_offset` to FLuffer's internal 
     reader `read_offset`


##### Fluffer_Error_t Fluffer_enReadNext(Fluffer_Reader_t * psReader, Fluffer_Entry_t * psEntry)

  Reads next entry
  
   - Set entry's id
  
   - Read from reader's read_offset `FLUFFER_ENTRY_SIZE` bytes
     into entry's data buffer
  
   - increment reader's `read_offset`


##### Fluffer_Error_t Fluffer_enMarkEntry(Fluffer_Entry_t * psEntry)

  Marks an entry
  
   - Get entry's address from entry's id
  
   - if entry's address is not valid, 
     return FLUFFER_ERROR_NOT_FOUND
  
   - if entry is not found in buffer, 
     return FLUFFER_ERROR_NOT_FOUND
  
   - Otherwise, Write entry's mark flags


##### Fluffer_Error_t Fluffer_enWriteEntry(Fluffer_Entry_t * psEntry)

  Writes an entry to main buffer
  
   - Get entry address from internal FLuffer writer
  
   - write entry data to buffer
  
   - increment FLuffer writer offset
  
   - if there is no more space left main buffer to hold 
     a new entry, start clean up


### Private Types

##### Fluffer_t

```
+----------------------------------------------------+
| typedef struct __Fluffer_t {                       |
|     uint8_t memory_area[Fluffer_ALLOCATED_AREAS];  |
|     uint8_t main_buffer;                           |
|     uint32_t head;                       |
|     uint32_t tail;                       |
| } Fluffer_t;                                       |
+----------------------------------------------------+
```

  FLuffer object, keeps track of states of each area 
  (main buffer, secondary buffer), current main buffer area,
  and has an internal reader and writer objects that hold the 
  current read/write offsets
  
  - **memory_area**: contains the flags for each area 
    (which area) is the main buffer, and which is a
    secondary buffer.
  - **main_buffer**: main buffer area
  - **head**: buffer's head, start reading from here
  - **tail**: buffer's tail, start writing from here
  

### Private APIs

##### void Fluffer_vidPrepareFluffer(void)

  Prepares allocated FLuffer memory. Called only on 1st time 
  FLuffer is used. Initially, allocated memory could be erased.
  But there is a possibility it's not. So, the allocated memory
  will be erased

   - Erase both areas.

   - set area0 as main buffer.

   - set area1 as secondary buffer.

   - set main_buffer member of internal FLuffer object


##### Fluffer_Error_t Fluffer_enBrandArea(uint8_t u8Area)

  Sets area as a main buffer
 
   - check if area is available
  
   - set area as main buffer


##### Fluffer_Error_t Fluffer_enFindUnMarked(uint32_t * pu32Offset)

  Finds first unmarked entry's offset to read
  
   - Start from main buffer area.
  
   - read entry mark
  
   - if not marked, return the current offset
  
   - if marked, increment read_offset, go to step 2
  
   - if the main buffer has no unmarked entries, 
     return Fluffer_ERROR_NOT_FOUND


##### Fluffer_Error_t Fluffer_enFindEmpty(uint32_t * psOffset)

  Finds first empty location's offset to write to

   - Start from main buffer area.
  
   - read entry start
  
   - if not marked, return the current offset
  
   - if marked, increment read_offset, go to step 2
  
   - if the main buffer has no more empty area, 
     return Fluffer_ERROR_FULL

##### Fluffer_Error_t Fluffer_enCleanUp(void)

  Cleans up main buffer.
  
   - Start from main buffer read_offset,
  
   - Read entry
  
   - write entry to secondary buffer
  
   - repeat from step 2
  
   - when all unmarked entries are moved to secondary buffer,
     mark secondary buffer as main buffer
  
   - erase old main buffer


### Private Macros

##### FLUFFER_OFFSET_TO_ID(u32Offset)

Converts from offset to an ID


##### FLUFFER_ID_TO_OFFSET(u32Id)

Converts from ID to offset

