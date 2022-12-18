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
	*(HT_info*)data = (HT_info){1, fileDesc, fileName, buckets, 0, (BF_BLOCK_SIZE-sizeof(HT_block_info)) / sizeof(Record)};

    BF_Block_SetDirty(block);
	CALL_OR_DIE(BF_UnpinBlock(block));


    // // // create a specific number of blocks

    // for(int i = 0; i < buckets; i++) {
    //     BF_Block_Init(&block);
	//     CALL_OR_DIE(BF_AllocateBlock(fileDesc, block));

    //     int* data = (int*)(BF_Block_GetData(block));
	// 	data[0] = 0;
    //     BF_Block_SetDirty(block);
	//     CALL_OR_DIE(BF_UnpinBlock(block));
    // }
    
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
	

	// check if it is a hash file.
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

int HT_InsertEntry(HT_info* ht_info, Record record){
    return 0;
}

int HT_GetAllEntries(HT_info* ht_info, void *value ){
    return 0;
}