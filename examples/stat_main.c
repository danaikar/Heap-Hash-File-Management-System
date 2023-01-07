#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "bf.h"
#include "ht_table.h"
#include "sht_table.h"

#define RECORDS_NUM 200 // you can change it if you want
#define FILE_NAME "data.db"
#define INDEX_NAME "index.db"

#define CALL_OR_DIE(call)   \
{                           \
  BF_ErrorCode code = call; \
  if (code != BF_OK) {      \
    BF_PrintError(code);    \
    exit(code);             \
  }                         \
}


// HashStatistics open the file with the given name, identifies the file
// type and calls the appropriate function that prints the statistics 
int HashStatistics(char* filename) {
	BF_Block* block;
	BF_Block_Init(&block);
    int fd;
    CALL_OR_DIE(BF_OpenFile(filename, &fd));
    CALL_OR_DIE(BF_GetBlock(fd, 0, block));
	HT_info* data = (HT_info*)BF_Block_GetData(block);
    int fileType = data->fileType;
    BF_CloseFile(data->fileDesc);

    if (data->fileType == 1) {
        HT_HashStatistics(filename);
    }
    else if (data->fileType == 2) {
        SHT_HashStatistics(filename);
    }
    else {
        return -1;
    }
}



int main() {   
    srand(12569874);
    BF_Init(LRU);
    // Αρχικοποιήσεις
    HT_CreateFile(FILE_NAME,10);
    printf("HT CREATED\n");
    SHT_CreateSecondaryIndex(INDEX_NAME,10,FILE_NAME);
    HT_info* info = HT_OpenFile(FILE_NAME);
    SHT_info* index_info = SHT_OpenSecondaryIndex(INDEX_NAME);
    
    // Κάνουμε εισαγωγή τυχαίων εγγραφών τόσο στο αρχείο κατακερματισμού τις οποίες προσθέτουμε και στο δευτερεύον ευρετήριο
    printf("Insert Entries\n");

    for (int id = 0; id < RECORDS_NUM; ++id) {
        Record record = randomRecord();
        int block_id = HT_InsertEntry(info, record);
        SHT_SecondaryInsertEntry(index_info, record, block_id);
    }

    HashStatistics(INDEX_NAME);
    HashStatistics(FILE_NAME);
    BF_Close();     
}
