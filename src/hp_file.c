#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "hp_file.h"
#include "record.h"

#define CALL_BF(call)       \
{                           \
  BF_ErrorCode code = call; \
  if (code != BF_OK) {      \
    BF_PrintError(code);    \
    return HP_ERROR;        \
  }                         \
}


int HP_CreateFile(char *fileName) {

    int fd;
	char *heapID = "HeapFile", *data;
	BF_Block *block;
	BF_ErrorCode err;

	err = BF_CreateFile(fileName);	
	if(err != BF_OK){	
		BF_PrintError(err);
		return -1;
	}
	
	err = BF_OpenFile(fileName, &fd);  
	if(err != BF_OK){
		BF_PrintError(err);
		return -2;
	}


	// allocate a block 
	// this block will be the header
	BF_Block_Init(&block);
	err = BF_AllocateBlock(fd, block);
	if(err != BF_OK){
		BF_PrintError(err);
		return -3;
	}
	

	// copy a string-identifier "HeapFile" there
	data = BF_Block_GetData(block);		
	memcpy(data, heapID, 9);	



	BF_Block_SetDirty(block);
	
	err = BF_UnpinBlock(block);
	if(err != BF_OK){
		BF_PrintError(err);
		return -4;
	}
	
	err = BF_CloseFile(fd);
	if(err != BF_OK){
		BF_PrintError(err);
		return -5;
	}
	
	BF_Block_Destroy(&block);
 	return 100;
}







HP_info* HP_OpenFile(char *fileName) {

    char *data, testHeap[9];
	BF_Block *block;
	BF_ErrorCode err;
	int fd;

	if ((err = BF_OpenFile(fileName, &fd) != BF_OK) ) {
		BF_PrintError(err);
		return NULL;
	}

	// initialize a block as a copy of the header block
	BF_Block_Init(&block);	
	err = BF_GetBlock(fd, 0, block);	
	if(err != BF_OK){
		BF_PrintError(err);
		return NULL;
	}

	// check if it is a heap file.
	data = BF_Block_GetData(block);	
	memcpy(testHeap, data, 9);
	if(strcmp(testHeap, "HeapFile") != 0) {		
		printf("This file is not a HeapFile.\n");
		return NULL;
	}
	


	err = BF_UnpinBlock(block);
	if(err != BF_OK){
		BF_PrintError(err);
		return NULL;
	}	
	BF_Block_Destroy(&block);


	// Create the struct HP_info and initialize it 
	HP_info *info = malloc(sizeof(HP_info));
	info->IsHP = 1;
    info->fileDesc = fd;
	info->name = fileName;

	return info;
}





int HP_CloseFile( HP_info* hp_info ) {

	int fd = hp_info->fileDesc;

	int err;
	if((err = BF_CloseFile(fd)) != BF_OK) { 
		BF_PrintError(err);
		return -6;
	}
	printf("\nClosed file %d.\n",fd);

	// when a file is being closed we should free the allocated struct
	free(hp_info); 

    return 0;
}





int HP_InsertEntry(HP_info* hp_info, Record record) {

	int fd = hp_info->fileDesc;
	char *fileName = hp_info->name;

	BF_ErrorCode err;
	if ((err = BF_OpenFile(fileName, &fd) != BF_OK) ) {
		BF_PrintError(err);
		return -9;
	}

	int count;
	if(err = BF_GetBlockCounter(fd, &count) != BF_OK) { 
		BF_PrintError(err);
		return -7;
	}		
	printf("Num of blocks: %d\n", count);



	if(count == 1) {
		printf("Make a new block.\n");
		BF_Block *block;
		BF_Block_Init(&block);
		err = BF_AllocateBlock(fd, block);
		if(err != BF_OK){
			BF_PrintError(err);
			return -3;
		}

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
	printf(" == %ld\n", blockCapacity);

	if(blockCapacity == recordCount) {
		printf("Should create new\n");
		
		err = BF_AllocateBlock(fd, block);
		if(err != BF_OK){
			BF_PrintError(err);
			return -3;
		}
		
		BF_GetBlockCounter(fd, &count);
		BF_GetBlock(fd, count-1, block);
		int* data = (int*)(BF_Block_GetData(block));
		data[0] = 0;
		BF_Block_SetDirty(block); //TODO
		BF_UnpinBlock(block);
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





int HP_GetAllEntries(HP_info* hp_info, int value) {

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
		for (int j = 0; j < recCount; ++j) { 
			
			//if(rec[j].id == value) 
				printRecord(rec[j]);
		}
	}
	
	BF_UnpinBlock(block);
	BF_Block_Destroy(&block);
	BF_CloseFile(fd);
	BF_Close();

	return 0;
}

