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
        exit(code);           \
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
	

	// copy a string-identifier "HeapFile" there
	data = BF_Block_GetData(block);	
	*(HP_info*)data = (HP_info){0, fd, fileName};
	
	BF_Block_SetDirty(block);
	
	CALL_OR_DIE(BF_UnpinBlock(block));
	
	CALL_OR_DIE(BF_CloseFile(fd));
	
	BF_Block_Destroy(&block);
 	return 100;
}







HP_info* HP_OpenFile(char *fileName) {
    char testHeap[9];
	BF_Block *block;
	BF_ErrorCode err;
	int fd;

	CALL_OR_DIE(BF_OpenFile(fileName, &fd) != BF_OK);

	// initialize a block as a copy of the header block
	BF_Block_Init(&block);	
	CALL_OR_DIE(BF_GetBlock(fd, 0, block));	
	

	// check if it is a heap file.
	HP_info* data = (HP_info*)BF_Block_GetData(block);	
	if(data->fileType != 1) {		
		printf("This file is not a HashFile.\n");
		return NULL;
	}

	CALL_OR_DIE(BF_UnpinBlock(block));
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
	printf("Num of blocks: %d\n", count);


	if(count == 1) {
		printf("Make a new block.\n");
		BF_Block *block;
		BF_Block_Init(&block);
		CALL_OR_DIE(BF_AllocateBlock(fd, block));

		BF_GetBlock(fd, 1, block);
		int* data = (int*)(BF_Block_GetData(block));
		data[0] = 0;
		BF_Block_SetDirty(block); //TODO
		BF_UnpinBlock(block);		 
	}
	
	BF_Block *block;
	BF_Block_Init(&block);	
	BF_GetBlockCounter(fd, &count);
	BF_GetBlock(fd, count-1, block);

	void *data = BF_Block_GetData(block);
	int recordCount = *(int*)data;
	printf("data[0] = %d\n", recordCount);

	int blockCapacity = (BF_BLOCK_SIZE-sizeof(int))/(sizeof(Record));
	printf(" == %d\n", blockCapacity);

	if(blockCapacity == recordCount) {
		printf("Should create new\n");
		
		CALL_OR_DIE(BF_AllocateBlock(fd, block));
		
		BF_GetBlockCounter(fd, &count);
		BF_GetBlock(fd, count-1, block);
		int* data = (int*)(BF_Block_GetData(block));
		data[0] = 0;
		// BF_Block_SetDirty(block); //TODO
		// BF_UnpinBlock(block);
	}

	BF_GetBlockCounter(fd, &count);
	BF_GetBlock(fd, count-1, block);

	data = BF_Block_GetData(block);
	(*(int*)data)++;
	recordCount = *(int*)data;
	printf("data[0] = %d\n", recordCount);
	Record* recordPtr = data + sizeof(int);
	recordPtr[recordCount] = record;
	BF_Block_SetDirty(block); //TODO
	BF_UnpinBlock(block);

    return recordCount;
}



// TODO: na kanei return.......

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

		printf("Contents of Block %d\n\t",i);
		BF_GetBlock(fd, i, block);
		data = BF_Block_GetData(block);
		Record* rec = data+sizeof(int);
		
		int recCount = *(int*)data;
		for (int j = 1; j < recCount+1; ++j) { 
			
			if(rec[j].id == value) 
				printRecord(rec[j]);
		}
		ans++;
	}
	
	// BF_UnpinBlock(block);
	BF_Block_Destroy(&block);
	// BF_CloseFile(fd);
	// BF_Close();

	return ans;
}

