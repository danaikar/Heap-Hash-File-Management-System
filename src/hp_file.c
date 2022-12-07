#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "hp_file.h"
#define MAX_OPEN_FILES 20

#define CALL_BF(call)       \
{                           \
  BF_ErrorCode code = call; \
  if (code != BF_OK) {      \
    BF_PrintError(code);    \
    return HP_ERROR;        \
  }                         \
}



int HP_CreateFile(char *fileName, Record_Attribute attr){
    if(attr < 0 || attr > 3)
        return -3;

    if(BF_CreateFile(fileName) == BF_ERROR)
        return -1;

    int fd1;
    if(BF_OpenFile(fileName, &fd1) == BF_ERROR)
        return -2;

    BF_Block *block;
    BF_Block_Init(&block);

    if(BF_AllocateBlock(fd1, block) == BF_ERROR)
        return -4;

    char* data;
    data = BF_Block_GetData(block);

    int blkCnt = BF_GetBlockCounter(fd1, &blkCnt);
    printf("File has %d blocks\n", blkCnt);

    if(BF_GetBlock(fd1, blkCnt-1, block) < 0) {
        return -1;
    }

    // initialize the HP_info 
    HP_info info;
    info.attr = attr;
    info.IS_HP = 1;

    // and save it at the first block
    memcpy(block, &info, sizeof(HP_info));
    BF_Block_SetDirty(block);  

    
    blkCnt = BF_GetBlockCounter(fd1, &blkCnt);
    printf("File has %d blocks\n", blkCnt);


    
    if(BF_CloseFile(fd1) == BF_ERROR)
        return -5;
    
    return 100;
}





HP_info* HP_OpenFile(char *fileName){

    int fd1;

    if(BF_OpenFile(fileName, &fd1) == BF_ERROR)
        return NULL;
    printf("ok\n");

      



    HP_info *info = malloc(sizeof(HP_info));
    return info;
}




int HP_CloseFile( HP_info header_info ){
    return -1;
}

int HP_InsertEntry(HP_info header_info, Record record){
    return -1;
}

int HP_GetAllEntries(HP_info header_info, void *value ){
    return -1;
}

