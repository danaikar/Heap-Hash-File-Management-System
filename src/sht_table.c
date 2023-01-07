#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "sht_table.h"
#include "ht_table.h"
#include "record.h"

char name[15];

#define CALL_OR_DIE(call)       \
{                               \
    BF_ErrorCode code = call;   \
    if (code != BF_OK) {        \
        BF_PrintError(code);    \
        exit(-1);               \
    }                           \
}


uint hash_string(char* name) {
	// djb2 hash function, απλή, γρήγορη, και σε γενικές γραμμές αποδοτική
    uint hash = 5381;
    for (char* s = name; *s != '\0'; s++)
		hash = (hash << 5) + hash + *s;			// hash = (hash * 33) + *s. Το foo << 5 είναι γρηγορότερη εκδοχή του foo * 32.
    return hash;
}


typedef struct Entry {
	char name[15];
	int block;
} Entry;

int SHT_CreateSecondaryIndex(char *sfileName,  int buckets, char* fileName) {
	strcpy(name, fileName);
    int sfileDesc;
    BF_Block *block;
    char *data;

    CALL_OR_DIE(BF_CreateFile(sfileName));
    CALL_OR_DIE(BF_OpenFile(sfileName, &sfileDesc));

    // allocate a block 
	// this block will be the header
    BF_Block_Init(&block);
	CALL_OR_DIE(BF_AllocateBlock(sfileDesc, block));
	data = BF_Block_GetData(block);

	SHT_info temp;
	for (int i = 0; i < buckets; i++) {
		temp.bucket_end[i] = -1; // no blocks for this bucket yet
	}

	temp.fileType = 2;
	temp.fileDesc = sfileDesc;
	temp.name = sfileName;
	temp.buckets = buckets;
	temp.recordsPerBlock = (BF_BLOCK_SIZE-sizeof(SHT_block_info)) / sizeof(Entry);
	
	*(SHT_info*)data = temp;

    BF_Block_SetDirty(block);
	CALL_OR_DIE(BF_UnpinBlock(block));
	BF_Block_Destroy(&block);
	BF_CloseFile(sfileDesc);

    return 0;
}


SHT_info* SHT_OpenSecondaryIndex(char *indexName){
    BF_Block *block;
	BF_ErrorCode err;
	int fd;

	if(BF_OpenFile(indexName, &fd) != BF_OK) return NULL;

	// initialize a block as a copy of the header block
	BF_Block_Init(&block);	
	if(BF_GetBlock(fd, 0, block) != BF_OK)	return NULL;	
	

	// check if it is a hash file
	SHT_info* data = (SHT_info*)BF_Block_GetData(block);	
	if(data->fileType != 2) {		
		printf("This file is not a HashFile.\n");
		return NULL;
	}
	data->fileDesc = fd;
	BF_Block_SetDirty(block);
	// if(BF_UnpinBlock(block) != BF_OK)  return NULL;
	BF_Block_Destroy(&block);

	return data;
}


int SHT_CloseSecondaryIndex(SHT_info* SHT_info) {
	int fd = SHT_info->fileDesc;
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



int SHT_SecondaryInsertEntry(SHT_info* sht_info, Record record, int block_id) {
	int fd = sht_info->fileDesc;
	int count;
	char *fileName = sht_info->name;
	void* data;

	int recordBucket = hash_string(record.name) % sht_info->buckets;

	BF_Block *block;
	BF_Block_Init(&block);
	
	Entry entry;
	strcpy(entry.name, record.name);
	entry.block = block_id;

	int* last_block = &(sht_info->bucket_end[recordBucket]); 
	int previous = *last_block;
	int flag = 0;
	// if there is no block, create one
	if (*last_block == -1) {
		data = create_block(block, fd, recordBucket);  
		CALL_OR_DIE(BF_GetBlockCounter(fd, &count));
		*last_block = count-1; 
	}
	else {
		// check if last bucket block is full
		CALL_OR_DIE(BF_GetBlock(fd, *last_block, block));
		data = BF_Block_GetData(block);

		int blockCapacity = sht_info->recordsPerBlock;
		SHT_block_info* block_info = (SHT_block_info*)data;

		if (block_info->recordCnt >= blockCapacity) {
			data = create_block(block, fd, recordBucket);
			CALL_OR_DIE(BF_GetBlockCounter(fd, &count));
			*last_block = count-1;
		}
		else flag = 1;
	}

	Entry* entries = (Entry*)(data+sizeof(HT_block_info));
	HT_block_info* block_info = (HT_block_info*)data;
	 
	entries[block_info->recordCnt] = entry;
	block_info->recordCnt++;
	if (!flag)block_info->nextBlock = previous;
	BF_Block_SetDirty(block); 
	CALL_OR_DIE(BF_UnpinBlock(block));

	return sht_info->bucket_end[recordBucket];
}

static int searchBlock(HT_info* ht_info, int id, char* name) {
	BF_Block *block;

	BF_Block_Init(&block);

	CALL_OR_DIE(BF_GetBlock(ht_info->fileDesc, id, block));
	void* data = BF_Block_GetData(block);
	
	Record* records = (Record*)(data + sizeof(HT_block_info));
	HT_block_info* block_info = (HT_block_info*)data;

	for (int j = 0; j < block_info->recordCnt; j++) {
		if (!strcmp(name, records[j].name)) {
			printRecord(records[j]);
		}
	}
	BF_Block_Destroy(&block);
	return 1;
}

int SHT_SecondaryGetAllEntries(HT_info* ht_info, SHT_info* sht_info, char* name) {
	
	if (name == NULL) return -1;
	int ans = 0;
	BF_Block* block;
	BF_Block_Init(&block);
	int recordBucket = hash_string(name) % sht_info->buckets;
	int startingBlock = sht_info->bucket_end[recordBucket];

	for (int i = startingBlock; i != -1;) {
		CALL_OR_DIE(BF_GetBlock(sht_info->fileDesc, i, block));
	
		void* data = BF_Block_GetData(block);
		SHT_block_info* block_info = (SHT_block_info*)data;
		Entry* entries = (Entry*)(data + sizeof(SHT_block_info));
	
		for (int j=0; j < block_info->recordCnt; j++) {
			searchBlock(ht_info, entries[j].block, name);
		}

		CALL_OR_DIE(BF_UnpinBlock(block));
		printf("from %d to %d\n", i, block_info->nextBlock);
		i = block_info->nextBlock;
		ans++;
	}
	BF_Block_Destroy(&block);
	return ans;
}


int HashStatistics(char* filename) {
	if(strcmp(filename, name) == 0) {  
		return SHT_HashStatistics(filename);
	}
	else {  
		return HT_HashStatistics(filename);
  	}
}


int SHT_HashStatistics( char* filename ) {
	printf("\n-- Secondary Hash Table Stats --\n");
	
	HT_info *info = HT_OpenFile(filename);
	int fd = info->fileDesc;
	int count;

	BF_GetBlockCounter(fd, &count);
	printf("Total Blocks: %d\n", count);


	HT_CloseFile(info);
	return 0;
}