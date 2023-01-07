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
	// create an array with the last block of each bucket
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

	// initialize a block (header)
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
	BF_Block_Destroy(&block);

	return data;
}


int HT_CloseFile(HT_info* HT_info) {
	int fd = HT_info->fileDesc;
	CALL_OR_DIE(BF_CloseFile(fd));
	printf("Closed file %d.\n",fd);
    return 0;
}


//-------------------------------------------------------------------------//
static void* create_block(BF_Block *block, int fd, int recordBucket) {
	void* data;
	CALL_OR_DIE(BF_AllocateBlock(fd, block));
	data = (void*)(BF_Block_GetData(block));
	*(HT_block_info*)data = (HT_block_info){0, -1, recordBucket};
	return data;
}
//-------------------------------------------------------------------------//


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
		// if there is no block in this bucket
		data = create_block(block, fd, recordBucket);  
		CALL_OR_DIE(BF_GetBlockCounter(fd, &count));
		*last_block = count-1;
	}
	else {
		// check if last bucket is full
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

	// iterate the list of blocks 
	for (int i = startingBlock; i != -1;) {
		CALL_OR_DIE(BF_GetBlock(ht_info->fileDesc, i, block));
		void* data = BF_Block_GetData(block);
		HT_block_info* block_info = (HT_block_info*)data;
		Record* records = (Record*)(data + sizeof(HT_block_info));

		// and check if there is a record with id == value
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





//-----------------------------HASH STATISTICS-----------------------------//



int HT_HashStatistics( char* filename ) {
	printf("\n-- Primary Hash Table Stats --\n");

	HT_info *info = HT_OpenFile(filename);
	int fd = info->fileDesc;
	int count;
	
	BF_GetBlockCounter(fd, &count);
	printf("Total Blocks: %d\n", count); // question a

	BF_Block* block;
	BF_Block_Init(&block);
	
	int hasOverflow = 0;

	for (int i=0; i<info->buckets; i++) {
		int start = info->bucket_end[i];

		int min = 0x3f3f3f3f, max = 0, average = 0;
		int blocksVisited = 0;

		for (int j=start; j!=-1;) {
			CALL_OR_DIE(BF_GetBlock(fd, j, block));
			void* data = BF_Block_GetData(block);
			HT_block_info* block_info = (HT_block_info*)data;
			
			min = (block_info->recordCnt < min) ? block_info->recordCnt : min; 
			max = (block_info->recordCnt > max) ? block_info->recordCnt : max; 
			average += block_info->recordCnt;
			
			BF_UnpinBlock(block);
			j = block_info->nextBlock;
			blocksVisited++;
		} 
  
		if (min = 0x3f3f3f3 && max == 0)
			printf("Bucket: %d\n\tEmpty bucket :(\n", i);  // question b
		else 
			printf("Bucket: %d\n\tmin: %d\n\tmax: %d\n\taverage: %lf\n\toverflow blocks: %d\n", i, min, max, (double)(average)/blocksVisited, blocksVisited-1); // question c,d
		if (blocksVisited-1) hasOverflow++;
	}

	printf("\n\nblocks per bucket: %lf\n\n%d buckets have overflow blocks\n", ((double)(count-1)/info->buckets), hasOverflow);	// question d

	HT_CloseFile(info);
	return 0;
}