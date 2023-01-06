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
        exit(-1);             \
    }                         \
}


int HT_CreateFile(char *fileName,  int buckets) {
	int fileDesc;
    BF_Block *block;
    char *data;

    CALL_OR_DIE(BF_CreateFile(fileName));
    CALL_OR_DIE(BF_OpenFile(fileName, &fileDesc));
    
	// allocate a block 
	// this block will be the header
    BF_Block_Init(&block);
	CALL_OR_DIE(BF_AllocateBlock(fileDesc, block));

	data = BF_Block_GetData(block);
	
	HT_info temp;
	for (int i = 0; i < buckets; i++) {
		temp.bucket_end[i] = -1; // no blocks for this bucket yet
	}

	temp.fileType = 1;
	temp.fileDesc = fileDesc;
	temp.name = fileName;
	temp.buckets = buckets;
	temp.recordsPerBlock = (BF_BLOCK_SIZE-sizeof(HT_block_info)) / sizeof(Record);
	
	*(HT_info*)data = temp;

    BF_Block_SetDirty(block);
	CALL_OR_DIE(BF_UnpinBlock(block));
	BF_Block_Destroy(&block);
	BF_CloseFile(fileDesc);
 	return 0;
}


HT_info* HT_OpenFile(char *fileName) {
	BF_Block *block;
	BF_ErrorCode err;
	int fd;

	if(BF_OpenFile(fileName, &fd) != BF_OK) return NULL;

	// initialize a block as a copy of the header block
	BF_Block_Init(&block);	
	if(BF_GetBlock(fd, 0, block) != BF_OK)	return NULL;	
	

	// check if it is a hash file
	HT_info* data = (HT_info*)BF_Block_GetData(block);	
	if(data->fileType != 1) {		
		printf("This file is not a HashFile.\n");
		return NULL;
	}

	data->fileDesc = fd;
	BF_Block_SetDirty(block);
	// if(BF_UnpinBlock(block) != BF_OK)  return NULL;
	BF_Block_Destroy(&block);

	return data;
}


int HT_CloseFile(HT_info* HT_info) {
	int fd = HT_info->fileDesc;
	CALL_OR_DIE(BF_CloseFile(fd));
	printf("Closed file %d.\n",fd);
    return 0;
}

static void* create_block(BF_Block *block, int fd, int recordBucket) {
	void* data;
	CALL_OR_DIE(BF_AllocateBlock(fd, block));
	data = (void*)(BF_Block_GetData(block));
	*(HT_block_info*)data = (HT_block_info){0, -1, recordBucket};
	return data;
}

static void print_blocks(HT_info* ht_info) {
	int count;
	char *fileName = ht_info->name;
	int fd = ht_info->fileDesc;
	
	BF_Block *block;
	BF_Block_Init(&block);

	BF_GetBlockCounter(fd, &count);

	for (int i=1; i<count; i++) {
		BF_GetBlock(fd, i, block);
		void* data = (void*)(BF_Block_GetData(block));
		HT_block_info* block_info = data;
		for (int j=0; j<block_info->recordCnt; j++) {
			Record* records = (Record*)(data+sizeof(HT_block_info));
			printRecord(records[j]);
		}
	}
}

int HT_InsertEntry(HT_info* ht_info, Record record) {
    int fd = ht_info->fileDesc;
	int count;
	char *fileName = ht_info->name;
	void* data;

	int recordBucket = record.id % ht_info->buckets;

	BF_Block *block;
	BF_Block_Init(&block);

	int* last_block = &(ht_info->bucket_end[recordBucket]); 
	int previous = *last_block;
	int flag = 0;
	if (*last_block == -1) {
		data = create_block(block, fd, recordBucket);  
		CALL_OR_DIE(BF_GetBlockCounter(fd, &count));
		*last_block = count-1;
	}
	else {
		// check if last bucket block is full
		CALL_OR_DIE(BF_GetBlock(fd, *last_block, block));
		data = BF_Block_GetData(block);

		int blockCapacity = ht_info->recordsPerBlock;
		HT_block_info* block_info = (HT_block_info*)data;

		if (block_info->recordCnt >= blockCapacity) {
			data = create_block(block, fd, recordBucket);
			CALL_OR_DIE(BF_GetBlockCounter(fd, &count));
			*last_block = count-1;
		}
		else flag = 1;
	}

	Record* records = (Record*)(data+sizeof(HT_block_info));
	HT_block_info* block_info = (HT_block_info*)data;
	 
	records[block_info->recordCnt] = record;
	block_info->recordCnt++;
	if (!flag) block_info->nextBlock = previous;
	
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
	int  startingBlock = ht_info->bucket_end[recordBucket];

	for (int i = startingBlock; i != -1;) {
		CALL_OR_DIE(BF_GetBlock(ht_info->fileDesc, i, block));
		void* data = BF_Block_GetData(block);
		HT_block_info* block_info = (HT_block_info*)data;
		Record* records = (Record*)(data + sizeof(HT_block_info));

		for (int j = 0; j < block_info->recordCnt; j++) {
			if (records[j].id == *(int*)value) {
				printRecord(records[j]);
			}
		}
		CALL_OR_DIE(BF_UnpinBlock(block));
		i = block_info->nextBlock;
		ans++;
	}

	BF_Block_Destroy(&block);
	return ans;
}



int HT_HashStatistics( char* filename ) {
	printf("\nht stat\n");

	HT_info *info = HT_OpenIndex(filename);
	int fd = info->fileDesc;
	

	HT_CloseFile(info);
	return 0;
}