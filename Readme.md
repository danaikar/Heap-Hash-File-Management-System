**About:** 
This DBMS project implements two primary storage mechanisms: heap files and hash-based files. It enables efficient record management by providing a range of functionalities, including file creation, entry insertion, and retrieval of records through both primary and secondary indexing. The system is developed with a focus on structured data access, making it suitable for educational purposes or as a foundation for database system prototypes.

**Functions in DBMS Project:**
**HP_CreateFile()**
The `HP_CreateFile` function creates and initializes an empty heap file with the given name. It sets up a header block containing file type, file ID, and name. Returns 0 on success, otherwise -1.

**HP_OpenFile()**
This function opens the heap file and reads the header. If valid, it returns a pointer to the header. On failure, returns `NULL`.

**HP_CloseFile()**
Closes the file. Returns 0 on success, otherwise -1.

**HP_InsertEntry()**
Inserts a record into the heap file. Allocates new blocks if necessary. Returns the block number on success, otherwise -1.

**HP_GetAllEntries()**
Searches and prints records matching a given ID. Returns the number of blocks read or -1 on error.

---

**HT_CreateFile()**
Creates and initializes a hash file with the given parameters. Returns 0 on success, otherwise -1.

**HT_OpenFile()**
Opens a hash file and reads its header. Returns a pointer to the header if successful, otherwise `NULL`.

**HT_CloseFile()**
Closes the hash file. Returns 0 on success, otherwise -1.

**HT_InsertEntry()**
Inserts a record into the appropriate bucket using hashing. Allocates new blocks if needed. Returns the block number on success, otherwise -1.

**HT_GetAllEntries()**
Finds and prints records matching a hash value. Returns the number of blocks read or -1 on error.

---

**SHT_CreateFile()**
Creates and initializes a secondary index hash file. Returns 0 on success, otherwise -1.

**SHT_OpenSecondaryIndex()**
Opens a secondary index hash file. Returns a pointer to the header if successful, otherwise `NULL`.

**SHT_CloseSecondaryIndex()**
Closes the secondary index file. Returns 0 on success, otherwise -1.

**SHT_InsertEntry()**
Inserts a record into the secondary index using hashing. Allocates new blocks if necessary. Returns the block number on success, otherwise -1.

**SHT_GetAllEntries()**
Searches for and prints records from the secondary index. Returns the number of blocks read or -1 on error.

---

**stat_main.c**
Handles the creation of hash files and inserts records. It calls `HashStatistics` to generate statistics for both HT and SHT files, depending on the file type.
