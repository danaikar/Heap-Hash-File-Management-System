#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "ht_table.h"
#include "record.h"
 


#define CALL_OR_DIE(call)     \
{                             \
    BF_ErrorCode code = call; \
    if (code != BF_OK) {      \
        BF_PrintError(code);  \
        exit(code);           \
    }                         \
}


int HT_CreateFile(char *fileName,  int buckets) {
	int fileDesc;
    BF_Block *block;
    char *hashID = "HashFile", *data;

    CALL_OR_DIE(BF_CreateFile(fileName));
    CALL_OR_DIE(BF_OpenFile(fileName, &fileDesc));
    
	// allocate a block 
	// this block will be the header
    BF_Block_Init(&block);
	CALL_OR_DIE(BF_AllocateBlock(fileDesc, block));

    // copy a string-identifier "HashFile" there
	data = BF_Block_GetData(block);
	
	HT_info temp;
	
	for (int i=0; i<buckets; i++) {
		temp.bucket_end[i] = i+1;
	}

	temp.fileType = 1;
	temp.fileDesc = fileDesc;
	temp.name = fileName;
	temp.buckets = buckets;
	temp.headerBlock = 0;
	temp.recordsPerBlock = (BF_BLOCK_SIZE-sizeof(HT_block_info)) / sizeof(Record);
	
	*(HT_info*)data = temp;

    BF_Block_SetDirty(block);
	CALL_OR_DIE(BF_UnpinBlock(block));

	for (int i=1; i<=buckets; i++) {
		CALL_OR_DIE(BF_AllocateBlock(fileDesc, block));
		data = BF_Block_GetData(block);
		*(HT_block_info*)data = (HT_block_info){0, -1, i-1};
		BF_Block_SetDirty(block);
		CALL_OR_DIE(BF_UnpinBlock(block));
	}

	BF_Block_Destroy(&block);
 	return 1;
}


HT_info* HT_OpenFile(char *fileName) {
	BF_Block *block;
	BF_ErrorCode err;
	int fd;

	CALL_OR_DIE(BF_OpenFile(fileName, &fd) != BF_OK);

	// initialize a block as a copy of the header block
	BF_Block_Init(&block);	
	CALL_OR_DIE(BF_GetBlock(fd, 0, block));	
	

	// check if it is a hash file
	HT_info* data = (HT_info*)BF_Block_GetData(block);	
	if(data->fileType != 1) {		
		printf("This file is not a HashFile.\n");
		return NULL;
	}
	
	CALL_OR_DIE(BF_UnpinBlock(block));
	BF_Block_Destroy(&block);

	return data;
}


int HT_CloseFile(HT_info* HT_info) {
	int fd = HT_info->fileDesc;
	CALL_OR_DIE(BF_CloseFile(fd));
	printf("\nClosed file %d.\n",fd);
    return 0;
}


int HT_InsertEntry(HT_info* ht_info, Record record) {
    int fd = ht_info->fileDesc;
	char *fileName = ht_info->name;
	// CALL_OR_DIE(BF_OpenFile(fileName, &fd));

	int recordBucket = record.id % ht_info->buckets;

	BF_Block *block;
	BF_Block_Init(&block);
	

	CALL_OR_DIE(BF_GetBlock(fd, ht_info->bucket_end[recordBucket], block));

	void* data = BF_Block_GetData(block);

	// check if last bucket block is full
	int blockCapacity = (BF_BLOCK_SIZE-sizeof(HT_block_info))/sizeof(record);
	HT_block_info* block_info = (HT_block_info*)data;
	Record* records = (Record*)(data+sizeof(HT_block_info));

	if (block_info->recordCnt < blockCapacity) {
		records[block_info->recordCnt] = record;
		block_info->recordCnt++;
	}
	else {
		int count;
		CALL_OR_DIE(BF_GetBlockCounter(fd, &count));
		block_info->nextBlock = count;

		CALL_OR_DIE(BF_AllocateBlock(fd, block));
		data = (void*)(BF_Block_GetData(block));
		*(HT_block_info*)data = (HT_block_info){1, -1, recordBucket};
		records = (Record*)(data+sizeof(HT_block_info));
		records[0] = record;
		ht_info->bucket_end[recordBucket] = count-1;
	}

	BF_Block_SetDirty(block); 
	CALL_OR_DIE(BF_UnpinBlock(block));

	return ht_info->bucket_end[recordBucket];
}


int HT_GetAllEntries(HT_info* ht_info, void *value) {
	if (value == NULL) return -1;
	int ans = 0;
	BF_Block* block;
	BF_Block_Init(&block);

	int recordBucket = (*(int*)value) % ht_info->buckets;
	int startingBlock = recordBucket + 1;

	for (int i = startingBlock; i != -1;) {
		printf("SEARCHING %d\n", i);
		CALL_OR_DIE(BF_GetBlock(ht_info->fileDesc, i, block));
		void* data = BF_Block_GetData(block);
		HT_block_info* block_info = (HT_block_info*)data;
		Record* records = (Record*)(data + sizeof(HT_block_info));

		for (int j = 0; j < block_info->recordCnt; j++) {
			if (records[j].id == *(int*)value) {
				printRecord(records[j]);
			}
		}
		i = block_info->nextBlock;
		ans++;
	}

	BF_Block_Destroy(&block);
	return ans;
}