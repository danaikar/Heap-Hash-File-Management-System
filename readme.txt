About: 
This DBMS project implements two primary storage mechanisms: heap files and hash-based files. It enables efficient record management by providing a range of functionalities, including file creation, entry insertion, and retrieval of records through both primary and secondary indexing. The system is developed with a focus on structured data access, making it suitable for educational purposes or as a foundation for database system prototypes.

#Functions in DBMS Project

##HP_CreateFile()
The `HP_CreateFile` function creates and initializes an empty heap file with the given name. It allocates and initializes a block to serve as the header, containing the file type (coded as 0), the file ID, and the name. If successful, it returns 0; otherwise, -1.

## HP_OpenFile()
The `HP_OpenFile` function opens the created heap file and reads the header from the first block. If the file is indeed a heap file, it returns a pointer to the header. On error, it returns `NULL`.

## HP_CloseFile()
This function closes the heap file. If successful, it returns 0; otherwise, -1.

## HP_InsertEntry()
The `HP_InsertEntry` function opens the file and checks how many blocks it contains. If the file contains only the header or if the current block is full, a new block is created. The record is then stored. It returns the block number on success, otherwise -1.

## HP_GetAllEntries()
This function opens the file, counts the blocks, and iterates through them (excluding the header) to find a record with the given ID. It prints the record if found and returns the number of blocks read. On error, it returns -1.

---

## HT_CreateFile()
The `HT_CreateFile` function creates and initializes an empty hash file with a given name, bucket count, and records per block. The buckets are represented as a list of blocks, and the end of each bucket list is stored in `bucket_end`. On success, it returns 0, otherwise -1.

## HT_OpenFile()
The `HT_OpenFile` function opens the hash file and reads the header. If it is a hash file, it returns a pointer to the header. On error, it returns `NULL`.

## HT_CloseFile()
This function closes the hash file. If successful, it returns 0; otherwise, -1.

## HT_InsertEntry()
The `HT_InsertEntry` function opens the file, hashes the record to find the corresponding bucket, and adds the record to the last block of that bucket. If necessary, a new block is allocated. It returns the block number on success, otherwise -1.

## HT_GetAllEntries()
This function opens the file, finds the corresponding bucket based on the given value, and searches backward through the blocks for a matching record. It prints the record if found and returns the number of blocks read. On error, it returns -1.

---

## SHT_CreateFile()
The `SHT_CreateFile` function creates and initializes an empty secondary index hash file with a given name, bucket count, and records per block. It returns 0 on success and -1 otherwise.

## SHT_OpenSecondaryIndex()
This function opens the secondary index hash file and reads the header. It returns a pointer to the header if successful, otherwise `NULL`.

## SHT_CloseSecondaryIndex()
The function closes the secondary index file. On success, it returns 0; otherwise -1.

## SHT_InsertEntry()
This function hashes the given string and finds the corresponding bucket. If needed, it allocates a new block and stores the record. It returns the block number on success, otherwise -1.

## SHT_GetAllEntries()
This function searches the secondary index for records with the given value and prints them. It returns the number of blocks read or -1 on error.

---

## Statistics Functions

The `stat_main.c` file handles the creation of the hash files and insertion of records. It calls the `HashStatistics` function to print statistics about the hash files (either HT or SHT) by reading the first block and calling the appropriate statistics function.

