#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "hp_file.h"
#include "record.h"

#define CALL_OR_DIE(call)     \
{                             \
    BF_ErrorCode code = call; \
    if (code != BF_OK) {      \
        BF_PrintError(code);  \
        exit(-1);           \
    }                         \
}


int HP_CreateFile(char *fileName) {

    int fd;
	char *data;
	BF_Block *block;
	BF_ErrorCode err;
	
	CALL_OR_DIE(BF_CreateFile(fileName));
	CALL_OR_DIE(BF_OpenFile(fileName, &fd));  
	
	// allocate a block 
	// this block will be the header
	BF_Block_Init(&block);
	CALL_OR_DIE(BF_AllocateBlock(fd, block));
	
	data = BF_Block_GetData(block);	
	*(HP_info*)data = (HP_info){0, fd, fileName};
	
	BF_Block_SetDirty(block);
	CALL_OR_DIE(BF_UnpinBlock(block));
	CALL_OR_DIE(BF_CloseFile(fd));
	BF_Block_Destroy(&block);
 	return 0;
}




HP_info* HP_OpenFile(char *fileName) {
    char testHeap[9];
	BF_Block *block;
	BF_ErrorCode err;
	int fd;

	if(BF_OpenFile(fileName, &fd) != BF_OK)  return NULL;

	// initialize a block (header)
	BF_Block_Init(&block);	
	if(BF_GetBlock(fd, 0, block) != BF_OK)	return NULL;	
	

	// check if it is a heap file
	HP_info* data = (HP_info*)BF_Block_GetData(block);	
	if(data->fileType != 0) {		
		printf("This file is not a HashFile.\n");
		return NULL;
	}

	if(BF_UnpinBlock(block) != BF_OK)  return NULL;
	BF_Block_Destroy(&block);

	return data;
}




int HP_CloseFile( HP_info* hp_info ) {
	int fd = hp_info->fileDesc;
	CALL_OR_DIE(BF_CloseFile(fd));
	printf("\nClosed file %d.\n",fd);
    return 0;
}




int HP_InsertEntry(HP_info* hp_info, Record record) {

	int fd = hp_info->fileDesc;
	char *fileName = hp_info->name;

	CALL_OR_DIE(BF_OpenFile(fileName, &fd));

	int count;
	CALL_OR_DIE(BF_GetBlockCounter(fd, &count));

	BF_Block *block;
	BF_Block_Init(&block);

	if(count == 1) {	// create a new block
		CALL_OR_DIE(BF_AllocateBlock(fd, block));
		BF_GetBlock(fd, 1, block);
		int* data = (int*)(BF_Block_GetData(block));
		*(HP_block_info*)data = (HP_block_info){0}; 
	}
		
	BF_GetBlockCounter(fd, &count);
	BF_GetBlock(fd, count-1, block);

	void *data = BF_Block_GetData(block);
	int recordCount = *(int*)data;

	int blockCapacity = (BF_BLOCK_SIZE-sizeof(HP_block_info))/(sizeof(Record));

	if(blockCapacity == recordCount) {
		// should create new block
		CALL_OR_DIE(BF_AllocateBlock(fd, block));
		
		BF_GetBlockCounter(fd, &count);
		BF_GetBlock(fd, count-1, block);
		int* data = (int*)(BF_Block_GetData(block));
		*(HP_block_info*)data = (HP_block_info){0};
	}

	BF_GetBlockCounter(fd, &count);
	BF_GetBlock(fd, count-1, block);

	data = BF_Block_GetData(block);
	(*(int*)data)++;
	recordCount = *(int*)data;

	Record* recordPtr = data + sizeof(HP_block_info);
	recordPtr[recordCount] = record;
	BF_Block_SetDirty(block);
	BF_UnpinBlock(block);

    return recordCount;
}



int HP_GetAllEntries(HP_info* hp_info, int value) {
	int ans = 0;
	BF_Block *block;
	BF_Block_Init(&block);
    void* data;

	int fd = hp_info->fileDesc;
	char *fileName = hp_info->name;

	BF_OpenFile(fileName, &fd);
	int blocks_num;
	BF_GetBlockCounter(fd, &blocks_num);

	for (int i = 1; i < blocks_num; ++i) {
		BF_GetBlock(fd, i, block);
		data = BF_Block_GetData(block);
		Record* rec = data + sizeof(HP_block_info);
		
		int recCount = *(int*)data;
		for (int j = 1; j < recCount+1; ++j) { 
			if(rec[j].id == value) 
				printRecord(rec[j]);
		}
		ans++;
	}
	
	BF_Block_Destroy(&block);
	return ans;
}