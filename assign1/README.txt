Project description:
Our project used C language to simulate a basic database system. For now, we used C to build a storage management system. 

Prerequisites:
Linux base system 
GCC command 
Make command

Running the tests:
After ran make command , use ./test_assign1_1 command to run test case. 



Function:
RC createPageFile,
This function creates and allocate space for a file. If we can not open or create file, we will return RC_FILE_NOT_FOUND. If file exist or we create a new file, we write "\0" to the file in page_size. 

RC openPageFile,
This function will open a exist file and store file info into SM_FileHandle structure. If can not open file, return RC_FILE_NOT_FOUND. If file exist and we open file, we store filename, file status, current page position, and total number page into fHandle. 

RC closePageFile, 
Close file. 

RC destroyPageFile,
Delete file.

RC readBlock,
First check if the file empty or not. If file empty, return RC_READ_NON_EXISTING_PAGE. Second, move the pointer to head of the file, and then we can read file. Finally, we should change current page info in fHandle.

RC getBlockPos
return block position

RC readFirstBlock
First check the file if empty or not. Then get to first block

RC readPreviousBlock
First check if current position is less than 1 then the page doesn't exist. Then go to previous block.

RC readCurrentBlock
check current page exist or not, if not return error. Then get to current block.

RC readNextBlock,
First check total number pages of the file. If current page position equals to total number pages, this function will return RC_READ_NON_EXISTING_PAGE. The other is same with RC readBlock.

RC readLastBlock, 
Get to last block position and move stream pointer.

RC writeBlock
First, move stream pointer to the right position. Second, check if fwrite writes one page size or not, if not return error. Third move stream pointer to the end of the file. Next, update fHandel info.

RC writeCurrentBlock
go to current block and run RC writeBlock function.

RC appendEmptyBlock
add a empty block.

RC ensureCapacity
First get the difference between current total pages number and the page number we want. If difference > 0, then we increase the size.

For the test file, we modified it by adding more method to test the extra function which haven't been tested in the precvious version of the test.