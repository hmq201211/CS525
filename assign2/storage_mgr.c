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

RC createPageFile (char *fileName) {
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

RC openPageFile (char *fileName, SM_FileHandle *fHandle) {
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
        fclose(fp);
        return RC_OK;
    }
    return RC_FILE_NOT_FOUND;
	
}

RC closePageFile (SM_FileHandle *fHandle) {
    int result = fclose(fHandle->mgmtInfo);// check fclose status
    return result == 0 ? RC_OK : RC_FILE_NOT_FOUND;// if successful then return OK, else return RC_FILE_NOT_FOUND
}


RC destroyPageFile (char *fileName) {
    int result = remove(fileName);// check remove status
    return result == 0 ? RC_OK : RC_FILE_NOT_FOUND;// if successful then return OK, else return RC_FILE_NOT_FOUND
}

RC readBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
    
    if(pageNum > fHandle->totalNumPages || pageNum < 0)
        return RC_READ_NON_EXISTING_PAGE;
    fp = fopen(fHandle->fileName, "r");
    if(fp == NULL)
        return RC_FILE_NOT_FOUND;
    if(fseek(fp, pageNum * PAGE_SIZE, SEEK_SET)!=0){
        return RC_READ_NON_EXISTING_PAGE;
    }else{
        if(fread(memPage,sizeof(char), PAGE_SIZE, fp)<PAGE_SIZE){
            return RC_ERROR;
        }
    }
    //fseek(fp, pageNum*PAGE_SIZE , SEEK_SET);
    fHandle->curPagePos = ftell(fp);
    fclose(fp);
    return RC_OK;
}

int getBlockPos (SM_FileHandle *fHandle) {
	return fHandle->curPagePos;
}

RC readFirstBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) {
    if (fHandle->totalNumPages <= 0)// check the file is empty or not
        return RC_READ_NON_EXISTING_PAGE;
    fseek(fp, 0, SEEK_SET);// move the pointer to the head of the file
    fread(memPage, 1, PAGE_SIZE, fp);// read
    fHandle->curPagePos = 0;// update information
    return RC_OK;
}

RC readPreviousBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) {
    if (fHandle->curPagePos < 1)// check if current position is less than 1 then the page doesn't exist
        return RC_READ_NON_EXISTING_PAGE;
    RC previousBlockPos = getBlockPos(fHandle);// call getBlockPos function to get current position
    fseek(fp, -PAGE_SIZE, SEEK_CUR);// move the pointer 1 page_size before the current position
    fread(memPage, 1, PAGE_SIZE, fp);// read
    fHandle->curPagePos = previousBlockPos - 1;// update information
    return RC_OK;
}

RC readCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) {
    if ((fHandle->curPagePos) > (fHandle->totalNumPages - 1))// check current page exist or not, if not return error
        return RC_READ_NON_EXISTING_PAGE;
    RC currentBlockPos = getBlockPos(fHandle);// call getBlockPos function to get current position
    fseek(fp, currentBlockPos * PAGE_SIZE, SEEK_SET);// move the pointer to the corresponding position
    fread(memPage, 1, PAGE_SIZE, fp);//read
    fHandle->curPagePos = currentBlockPos;// update fHandle information
    return RC_OK;
}

RC readNextBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
    if (fHandle->curPagePos > (fHandle->totalNumPages - 1))
        return RC_READ_NON_EXISTING_PAGE;
    RC nextBlockPos = getBlockPos(fHandle) + 1;// use getBlockPos to get nextBlockPos value
    fseek(fp, nextBlockPos * PAGE_SIZE, SEEK_SET);// move pointer
    fread(memPage, 1, PAGE_SIZE, fp);// read
    fHandle->curPagePos = nextBlockPos;//update information
    return RC_OK;
}

RC readLastBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
    RC lastBlockPos = fHandle->totalNumPages - 1;// get last block position
    fseek(fp, lastBlockPos * PAGE_SIZE, SEEK_SET);// move stream pointer
    fread(memPage, 1, PAGE_SIZE, fp);// read
    fHandle->curPagePos = lastBlockPos;// update fHandle information
    return RC_OK;
}

RC writeBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
	if (pageNum > fHandle->totalNumPages || pageNum < 0)
        	return RC_WRITE_FAILED;
	fp = fopen(fHandle->fileName, "r+");
	if(fp == NULL)
		return RC_FILE_NOT_FOUND;
	if(pageNum != 0) {
        fHandle->curPagePos = pageNum * PAGE_SIZE;
        fclose(fp);
        writeCurrentBlock(fHandle, memPage);
	} else {
        fseek(fp, pageNum * PAGE_SIZE, SEEK_SET);
        int i;
        for(i = 0; i < PAGE_SIZE; i++)
        {
            if(feof(fp))
                appendEmptyBlock(fHandle);
            fputc(memPage[i], fp);
        }
        fHandle->curPagePos = ftell(fHandle->mgmtInfo)/PAGE_SIZE;
        fclose(fp);
	}
	return RC_OK;
}

RC writeCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) {
	fp = fopen(fHandle->fileName, "r+");
	if(fp == NULL)
		return RC_FILE_NOT_FOUND;
	appendEmptyBlock(fHandle);
	fseek(fp, fHandle->curPagePos, SEEK_SET);
	fwrite(memPage, sizeof(char), strlen(memPage), fp);
    fHandle->curPagePos = ftell(fp);
    fclose(fp);
	return RC_OK;
}


RC appendEmptyBlock (SM_FileHandle *fHandle) {
    SM_PageHandle Block = (SM_PageHandle)calloc(PAGE_SIZE, sizeof(char));
    if(fseek(fp, 0, SEEK_END)!=0){
        free(Block);
        return RC_WRITE_FAILED;
    }else{
        fwrite(Block, sizeof(char), PAGE_SIZE, fp);
    }
    //update fHandle information
    fHandle->totalNumPages++;
    free(Block);
    return RC_OK;
}

RC ensureCapacity (int numberOfPages, SM_FileHandle *fHandle) {
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



