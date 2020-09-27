Project description:
In this assignment we implement a buffer manager in the system.
The buffer manager manages a fixed number of pages in memory that represent pages from a page file managed by the storage manager implemented in assignment 1. 

Prerequisites:
Linux base system 
GCC command 
Make command

Running the tests:
After ran make command , use ./test1 for the default test case.
We also added command for an extra test case. We named it "test_assign2_2"
If you want to run an extra test case, please do the following steps:
1. rename it as "test_assign2_2".
2. run "make test2"
3. run "./test2"
you can also use "make clean" clean the old o files.

Function:
RC initBufferPool:
This function is used to initialize the Buffer Pool.

RC shutdownBufferPool
this function is used to shutdown the buffer pool

RC forceFlushPool
this function is used to write back the data in all dirty pages in the Buffer Pool

// Buffer Manager Interface Access Pages
RC markDirty 
this function is used to mark the requested page as dirty

RC unpinPage 
this function is used to unpin a page

RC forcePage 
this function is used to write the requested page back to the page file on disk

RC pinPage 
This function is used to pin the page with the requested pageNum in the BUffer Pool.If the page is not in the Buffer Pool, load it from the file to the Buffer Pool


// Statistics Interface
PageNumber *getFrameContents 
To get an array of PageNumbers .An empty page frame is represented using the constant NO_PAGE

bool *getDirtyFlags 
To get an array of bools .Empty page frames are considered as clean

int *getFixCounts 
To get an array of ints .

int getNumReadIO 
To get the number of pages that have been read from disk.since the Buffer Pool has been initialized

int getNumWriteIO 
To get the number of pages written to the page file.since the Buffer Pool has been initialized


