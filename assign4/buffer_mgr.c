#include<stdio.h>
#include<stdlib.h>
#include "buffer_mgr.h"
#include "storage_mgr.h"
#include <math.h>


typedef struct Page // one page frame in buffer pool
{
	SM_PageHandle data; // data
	PageNumber pageNum;// used to distinguish different pages
	int dirtyFlag; // modified or not
	int fixCountNumber; // number of clients in use
	int hitCountNumber;   // attribute used in replacement method
} PageFrame;


int bufferPoolSize = 0; // size of buffer pool

int lastIndex = 0;// FIFO attribute to show the count of pages which read from disk

int writeCountNumber = 0;// count for write operation

int hitCount = 0;// attribute which helps in LRU method

int clockPointer = 0;// attribute which helps in clock method

int match(BM_PageHandle *const page, PageFrame *pg) { //find pageNumber in the pool
	int i;
	for (i = 0; i < bufferPoolSize; i++) {
		if (pg[i].pageNum == page->pageNum) {
			return i;
		}
	}
	return -1;
}

void helpher(PageFrame *pf, const PageNumber pn, BM_BufferPool *const bm) {// assign values to the page frame
	SM_FileHandle fh;
	openPageFile(bm->pageFile, &fh);
	pf->data = (SM_PageHandle) malloc(PAGE_SIZE);
	readBlock(pn, &fh, pf->data);
	pf->pageNum = pn;
	pf->dirtyFlag = 0;
	pf->fixCountNumber = 1;
	lastIndex++;
	hitCount++;
}

void package(PageFrame *pf, int index, PageFrame *page) {
	pf[index].data = page->data;
	pf[index].pageNum = page->pageNum;
	pf[index].dirtyFlag = page->dirtyFlag;
	pf[index].fixCountNumber = page->fixCountNumber;
}

void writetomemory(BM_BufferPool *const bm, PageFrame *pf, int index) {//write form buffer pool back to disk
	SM_FileHandle fh;
	openPageFile(bm->pageFile, &fh);
	writeBlock(pf[index].pageNum, &fh, pf[index].data);
	writeCountNumber++;
}


extern void FIFO(BM_BufferPool *const bm, PageFrame *page)// First In First Out replacement method
{
	PageFrame *pageFrame = (PageFrame *) bm->mgmtData;
	int i;
	int index = lastIndex % bufferPoolSize;



	for (i = 0; i < bufferPoolSize; i++) {
		if (pageFrame[index].fixCountNumber != 0) {
			index++;
			index = index % bufferPoolSize;
		} else {
			if (pageFrame[index].dirtyFlag == 1) {
				writetomemory(bm, pageFrame, index);

			}
			package(pageFrame, index, page);
			break;
		}
	}

}


extern RC LRU(BM_BufferPool *const bm, PageFrame *page) //Least Recently Used Replacement Method
{
	PageFrame *pageFrame = (PageFrame *) bm->mgmtData;
	int i;
	int leastHitIndex = 0;
	int leastHitNum = pageFrame[0].hitCountNumber;



	for (i = 0; i < bufferPoolSize; i++) {
		if (pageFrame[i].hitCountNumber < leastHitNum) {
			leastHitIndex = i;
			leastHitNum = pageFrame[i].hitCountNumber;
		}
	}
	if (pageFrame[leastHitIndex].dirtyFlag == 1) {
		writetomemory(bm, pageFrame, leastHitIndex);

	}

	package(pageFrame, leastHitIndex, page);
	pageFrame[leastHitIndex].hitCountNumber = page->hitCountNumber;

	return RC_OK;
}


extern void CLOCK(BM_BufferPool *const bm, PageFrame *page)// Clock replacement method
{
	PageFrame *pageFrame = (PageFrame *) bm->mgmtData;
	while (1) {
		clockPointer = clockPointer % bufferPoolSize;
		if (pageFrame[clockPointer].hitCountNumber != 0) {
			pageFrame[clockPointer++].hitCountNumber = 0;
		} else {
			if (pageFrame[clockPointer].dirtyFlag == 1) {
				writetomemory(bm, pageFrame, clockPointer);
			}
			package(pageFrame, clockPointer, page);
			pageFrame[clockPointer].hitCountNumber = page->hitCountNumber;
			clockPointer++;
			break;
		}
	}
}

extern RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName, // init
						 const int numPages, ReplacementStrategy strategy,
						 void *stratData) {
	bm->pageFile = (char *) pageFileName;
	bm->numPages = numPages;
	bm->strategy = strategy;
	PageFrame *page = malloc(sizeof(PageFrame) * numPages);
	bm->mgmtData = page;
	for (int i = 0; i < numPages; i++)// init all the page frames
	{
		page[i].data = NULL;
		page[i].pageNum = -1;
		page[i].dirtyFlag = 0;
		page[i].fixCountNumber = 0;
		page[i].hitCountNumber = 0;
	}
	writeCountNumber = clockPointer = 0;
	bufferPoolSize = numPages;
	return RC_OK;

}

extern RC shutdownBufferPool(BM_BufferPool *const bm)// close the buffer pool and release the memory
{
	PageFrame *pageFrame = (PageFrame *) bm->mgmtData;
	forceFlushPool(bm);
	int i;
	for (i = 0; i < bufferPoolSize; i++) {
		if (pageFrame[i].fixCountNumber != 0) {
			return RC_PINNED_PAGES_IN_BUFFER;
		}
	}
	free(pageFrame);
	bm->mgmtData = NULL;
	return RC_OK;
}

extern RC forceFlushPool(BM_BufferPool *const bm) // find all the modified frames and write them into disk
{
	PageFrame *pageFrame = (PageFrame *) bm->mgmtData;

	int i;
	for (i = 0; i < bufferPoolSize; i++) {
		if (pageFrame[i].fixCountNumber == 0 && pageFrame[i].dirtyFlag == 1) {
			writetomemory(bm, pageFrame, i);
			pageFrame[i].dirtyFlag = 0;
		}
	}
	return RC_OK;
}


extern RC markDirty(BM_BufferPool *const bm,
					BM_PageHandle *const page)// function used to make change the value of page modified or not
{
	PageFrame *pageFrame = (PageFrame *) bm->mgmtData;
	int i = match(page, pageFrame);
	if (i != -1) {
		pageFrame[i].dirtyFlag = 1;
		return RC_OK;
	}
	return RC_ERROR;
}

extern RC unpinPage(BM_BufferPool *const bm, BM_PageHandle *const page)// unpin a page in buffer pool
{
	PageFrame *pageFrame = (PageFrame *) bm->mgmtData;
	int i = match(page, pageFrame);
	if (i != -1) {
		pageFrame[i].fixCountNumber--;
		return RC_OK;
	}
	return RC_ERROR;
}

extern RC forcePage(BM_BufferPool *const bm, BM_PageHandle *const page) // write the modified page back into disk
{
	PageFrame *pageFrame = (PageFrame *) bm->mgmtData;
	//int i;

	int i = match(page, pageFrame);
	if (i != -1) {
		pageFrame[i].dirtyFlag = 0;
		writetomemory(bm, pageFrame, i);

		return RC_OK;
	}
	return RC_ERROR;
}

extern RC pinPage(BM_BufferPool *const bm, BM_PageHandle *const page, // pin a page into buffer pool
				  const PageNumber pageNum) {
	PageFrame *pageFrame = (PageFrame *) bm->mgmtData;
	if (pageFrame[0].pageNum != -1)// buffer pool is not empty
	{
		int i;
		bool isBufferFull = true;

		for (i = 0; i < bufferPoolSize; i++) {
			if (pageFrame[i].pageNum != -1) {
				if (pageFrame[i].pageNum == pageNum) {
					pageFrame[i].fixCountNumber++; // one more client accessing this page frame
					isBufferFull = false;
					hitCount++;

					if (bm->strategy == RS_LRU)
						pageFrame[i].hitCountNumber = hitCount;
					else if (bm->strategy == RS_CLOCK)
						pageFrame[i].hitCountNumber = 1;// last frame added to the buffer pool

					page->pageNum = pageNum;
					page->data = pageFrame[i].data;

					clockPointer++;
					break;
				}
			} else {
				SM_FileHandle fh;
				openPageFile(bm->pageFile, &fh);
				pageFrame[i].data = (SM_PageHandle) malloc(PAGE_SIZE);
				readBlock(pageNum, &fh, pageFrame[i].data);
				pageFrame[i].pageNum = pageNum;
				pageFrame[i].fixCountNumber = 1;
				lastIndex++;
				hitCount++;

				if (bm->strategy == RS_LRU)
					pageFrame[i].hitCountNumber = hitCount;
				else if (bm->strategy == RS_CLOCK)
					pageFrame[i].hitCountNumber = 1;

				page->pageNum = pageNum;
				page->data = pageFrame[i].data;

				isBufferFull = false;
				break;
			}
		}

		if (isBufferFull == true)//buffer pool is full
		{
			PageFrame *newPage = (PageFrame *) malloc(sizeof(PageFrame));// new page frame to hold the data
			helpher(newPage, pageNum, bm);

			page->pageNum = pageNum;
			page->data = newPage->data;

//Decide which replacement strategy to use based on the parameter
			if (bm->strategy == RS_FIFO) {
				FIFO(bm, newPage);
			} else if (bm->strategy == RS_LRU) {
				newPage->hitCountNumber = hitCount;
				LRU(bm, newPage);
			} else if (bm->strategy == RS_CLOCK) {
				newPage->hitCountNumber = 1;
				CLOCK(bm, newPage);
			} else {
				printf("method is not implemented");
			}

		}
		return RC_OK;
	} else// buffer pool is empty
	{
		SM_FileHandle fh;
		openPageFile(bm->pageFile, &fh);
		pageFrame[0].data = (SM_PageHandle) malloc(PAGE_SIZE);
		ensureCapacity(pageNum, &fh);
		readBlock(pageNum, &fh, pageFrame[0].data);
		pageFrame[0].pageNum = pageNum;
		pageFrame[0].fixCountNumber++;
		lastIndex = hitCount = 0;
		pageFrame[0].hitCountNumber = hitCount;
		page->pageNum = pageNum;
		page->data = pageFrame[0].data;
		return RC_OK;
	}
}


extern PageNumber *getFrameContents(BM_BufferPool *const bm) { // get the page names  in the buffer pool
	PageNumber *frameContents = malloc(sizeof(PageNumber) * bufferPoolSize);
	PageFrame *pageFrame = (PageFrame *) bm->mgmtData;

	int i;
	for (i = 0; i < bufferPoolSize; i++) {
		if (pageFrame[i].pageNum != -1) {
			frameContents[i] = pageFrame[i].pageNum;
		} else {
			frameContents[i] = NO_PAGE;
		}
	}
	return frameContents;
}

extern bool *getDirtyFlags(BM_BufferPool *const bm) { // get the page is modified or not for the pages in buffer pool
	bool *dirtyFlags = malloc(sizeof(bool) * bufferPoolSize);
	PageFrame *pageFrame = (PageFrame *) bm->mgmtData;

	int i;
	for (i = 0; i < bufferPoolSize; i++) {
		if (pageFrame[i].dirtyFlag == 1) {
			dirtyFlags[i] = true;
		} else {
			dirtyFlags[i] = false;
		}
	}
	return dirtyFlags;
}

extern int *getFixCounts(BM_BufferPool *const bm) { // get the fix number for every page frame in the buffer pool
	int *fixCounts = malloc(sizeof(int) * bufferPoolSize);
	PageFrame *pageFrame = (PageFrame *) bm->mgmtData;

	int i = 0;
	while (i < bufferPoolSize) {
		if (pageFrame[i].fixCountNumber != -1) {
			fixCounts[i] = pageFrame[i].fixCountNumber;
		} else {
			fixCounts[i] = 0;
		}
		i++;
	}
	return fixCounts;
}

extern int getNumReadIO(BM_BufferPool *const bm) {// get the count of read
	return (lastIndex + 1);
}

extern int getNumWriteIO(BM_BufferPool *const bm) {// return the count of write
	return writeCountNumber;
}
