#include "storage_mgr.h"
#include <sys/stat.h>
#include <stdio.h>

FILE *fp;// file pointer to the opened file

void initStorageManager(void) {
    printf("Storage Manager has been initialized!\n");
    printf("Team member: Mingqing Hou A20399118\n");
    printf("Team member: Bo Zhang A20360841\n");
    printf("Team member: Junjie Jiang A20380663\n");
}

RC createPageFile(char *fileName) {
    fp = fopen(fileName, "w");// init fp
    if (fp != NULL) {
        //fopen success, then write '\0' to the file in Page_Size
        fseek(fp, 0, SEEK_SET);
        for (int i = 0; i < PAGE_SIZE; ++i) {
            fwrite("\0", 1, 1, fp);
            fseek(fp, 0, SEEK_END);
        }
        return RC_OK;
    }
    return RC_FILE_NOT_FOUND;
}

RC openPageFile(char *fileName, SM_FileHandle *fHandle) {
    fp = fopen(fileName, "r+");//init fp
    if (fp != NULL) {
        //fopen success
        // declare a file stat variable
        struct stat fileStat;
        //get file's size
        stat(fileName, &fileStat);
        int file_Size = fileStat.st_size;
        //init fHandle's information
        fHandle->fileName = fileName;
        fHandle->mgmtInfo = fp;
        fHandle->curPagePos = ftell(fp) / PAGE_SIZE;
        fHandle->totalNumPages = file_Size / PAGE_SIZE;
        return RC_OK;
    }
    return RC_FILE_NOT_FOUND;
}

RC closePageFile(SM_FileHandle *fHandle) {
    int result = fclose(fHandle->mgmtInfo);// check fclose status
    return result == 0 ? RC_OK : RC_FILE_NOT_FOUND;// if successful then return OK, else return RC_FILE_NOT_FOUND
}

RC destroyPageFile(char *fileName) {
    int result = remove(fileName);// check remove status
    return result == 0 ? RC_OK : RC_FILE_NOT_FOUND;// if successful then return OK, else return RC_FILE_NOT_FOUND
}

RC readBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
    if (pageNum > fHandle->totalNumPages)// if there'are not enough left to read, then return RC_READ_NON_EXISTING_PAGE
        return RC_READ_NON_EXISTING_PAGE;
    fseek(fp, pageNum * PAGE_SIZE, SEEK_SET);// move the pointer to the corresponding position
    fread(memPage, 1, PAGE_SIZE, fp);//read
    fHandle->curPagePos = pageNum;// update fHandle information
    return RC_OK;
}

int getBlockPos(SM_FileHandle *fHandle) {
    return fHandle->curPagePos;
}

RC readFirstBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    if (fHandle->totalNumPages <= 0)// check the file is empty or not
        return RC_READ_NON_EXISTING_PAGE;
    fseek(fp, 0, SEEK_SET);// move the pointer to the head of the file
    fread(memPage, 1, PAGE_SIZE, fp);// read
    fHandle->curPagePos = 0;// update information
    return RC_OK;
}

RC readPreviousBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {

    if (fHandle->curPagePos < 1)// check if current position is less than 1 then the page doesn't exist
        return RC_READ_NON_EXISTING_PAGE;
    RC previousBlockPos = getBlockPos(fHandle);// call getBlockPos function to get current position
    fseek(fp, -PAGE_SIZE, SEEK_CUR);// move the pointer 1 page_size before the current position
    fread(memPage, 1, PAGE_SIZE, fp);// read
    fHandle->curPagePos = previousBlockPos - 1;// update information
    return RC_OK;
}

RC readCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    if ((fHandle->curPagePos) > (fHandle->totalNumPages - 1))// check current page exist or not, if not return error
        return RC_READ_NON_EXISTING_PAGE;
    RC currentBlockPos = getBlockPos(fHandle);// call getBlockPos function to get current position
    fseek(fp, currentBlockPos * PAGE_SIZE, SEEK_SET);// move the pointer to the corresponding position
    fread(memPage, 1, PAGE_SIZE, fp);//read
    fHandle->curPagePos = currentBlockPos;// update fHandle information
    return RC_OK;
}

RC readNextBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    if (fHandle->curPagePos > (fHandle->totalNumPages - 1))
        return RC_READ_NON_EXISTING_PAGE;
    RC nextBlockPos = getBlockPos(fHandle) + 1;// use getBlockPos to get nextBlockPos value
    fseek(fp, nextBlockPos * PAGE_SIZE, SEEK_SET);// move pointer
    fread(memPage, 1, PAGE_SIZE, fp);// read
    fHandle->curPagePos = nextBlockPos;//update information
    return RC_OK;
}

RC readLastBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    RC lastBlockPos = fHandle->totalNumPages - 1;// get last block position
    fseek(fp, lastBlockPos * PAGE_SIZE, SEEK_SET);// move stream pointer
    fread(memPage, 1, PAGE_SIZE, fp);// read
    fHandle->curPagePos = lastBlockPos;// update fHandle information
    return RC_OK;
}

RC writeBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
    fseek(fp, pageNum * PAGE_SIZE, SEEK_SET);// move stream pointer to the right position
    if (fwrite(memPage, PAGE_SIZE, 1, fp) != 1)// check if fwrite writes one page size or not, if not return error
        return RC_WRITE_FAILED;
    fseek(fp, 0, SEEK_END);// move stream pointer to the end of the file
    fHandle->curPagePos = pageNum;// update fHandle information
    fHandle->totalNumPages = (ftell(fp) / PAGE_SIZE);// update fHandle information
    return RC_OK;
}

RC writeCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {

    RC cur_position = getBlockPos(fHandle);
    return writeBlock(cur_position, fHandle, memPage); // call writeBlock function to do things

}

RC appendEmptyBlock(SM_FileHandle *fHandle) {
    //write one page size '\0'
    fseek(fp, 0, SEEK_END);
    for (int i = 0; i < PAGE_SIZE; i++) {
        fwrite("\0", 1, 1, fp);
        fseek(fp, 0, SEEK_END);
    }
    //update fHandle information
    fHandle->totalNumPages = (ftell(fp) / PAGE_SIZE);
    fHandle->curPagePos = fHandle->totalNumPages - 1;
    return RC_OK;
}

RC ensureCapacity(int numberOfPages, SM_FileHandle *fHandle) {
    int currentPageNumber = fHandle->totalNumPages;
    int page_difference = numberOfPages -
                          currentPageNumber;// get the difference between current total pages number and the page number we want
    if (page_difference > 0)
        // if the difference exists,call appendEmptyBlock function to append corresponding pages
        for (int i = 0; i < page_difference; i++)
            appendEmptyBlock(fHandle);
    return RC_OK;

}

//void main() {
//
////    //test createPageFile function
////    printf(createPageFile("test.txt"));
//
////    //test openPageFile function
////    SM_FileHandle fh;
////    printf(openPageFile("test.txt", &fh));
//
////    //test closePageFile function
////    printf(closePageFile(&fh));
//
////    //test destroyPageFile function
////    printf(destroyPageFile("test.txt"));
//
////    //test readBlock function
////    SM_FileHandle fh;
////    SM_PageHandle ph;
////    openPageFile("test.txt", &fh);
////    printf(readBlock(1, &fh, &ph));
//
////    //test getBlockPos function
////    SM_FileHandle fh;
////    SM_PageHandle ph;
////    openPageFile("test.txt", &fh);
////    readBlock(2, &fh, &ph)
////    printf(getBlockPos(&fh));
//
////    //test readFirstBlock function
////    SM_FileHandle fh;
////    SM_PageHandle ph;
////    openPageFile("test.txt", &fh);
////    printf(readFirstBlock( &fh, &ph));
//
////    //test readLastBlock function
////    SM_FileHandle fh;
////    SM_PageHandle ph;
////    openPageFile("test.txt", &fh);
////    printf(readLastBlock( &fh, &ph));
//
////    //test readCurrentBlock function
////    SM_FileHandle fh;
////    SM_PageHandle ph;
////    openPageFile("test.txt", &fh);
////    readLastBlock(&fh, &ph);
////    printf(readCurrentBlock(&fh, &ph));
//
////    //test readNextBlock function
////    SM_FileHandle fh;
////    SM_PageHandle ph;
////    openPageFile("test.txt", &fh);
////    readFirstBlock(&fh, &ph);
////    printf(readNextBlock(&fh, &ph));
//
////    //test readPreviousBlock function
////    SM_FileHandle fh;
////    SM_PageHandle ph;
////    openPageFile("test.txt", &fh);
////    readLastBlock(&fh, &ph);
////    printf(readPreviousBlock(&fh, &ph));
//
////    //test writeBlock function
////    SM_FileHandle fh;
////    openPageFile("test2.txt", &fh);
////    printf(writeBlock(1,&fh,"hhhhh"));
//
////    //test writeCurrentBlock function
////    SM_FileHandle fh;
////    openPageFile("test2.txt", &fh);
////    printf(writeCurrentBlock(&fh,"hhhhh"));
//
////    //test appendEmptyBlock function
////    SM_FileHandle fh;
////    openPageFile("test2.txt", &fh);
////    printf(appendEmptyBlock(&fh));
//
////    //test ensureCapacity function
////    SM_FileHandle fh;
////    openPageFile("test2.txt", &fh);
////    printf(ensureCapacity(4,&fh));
//
//}