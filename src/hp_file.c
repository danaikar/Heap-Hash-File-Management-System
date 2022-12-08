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
    return 0;
}

int HP_GetAllEntries(HP_info* hp_info, int value) {
   return 0;
}

