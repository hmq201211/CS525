Project description:
In this assignment we implement a record manager in the system.

Prerequisites:
Linux base system 
GCC command 
Make command

Running the tests:
After ran "make" command , use "make run_common_test" for the default test case.
you can also use "make clean" clean the old o files.
Also you can use "make expr_test" then use "make run_expr_test" to test expr


Function:
int findFreeSlot:
This function is created by us to find a free slot in a page.

RC initRecordManager
This function initializes the Record Manager

RC shutdownRecordManager
This functions shuts down the Record Manager

RC createTable 
This function creates a TABLE 

RC openTable
This function opens the table

RC closeTable 
This function closes the table

RC deleteTable
This function deletes the table 

int getNumTuples
This function returns the number of tuples(records) in the table 

RC insertRecord 
Insert a record to the table

RC deleteRecord
Delete a record with given RID

RC updateRecord
Update a table slot with the passed in record

RC getRecord
Get a record from a certain table and certain RID

RC startScan
This function scans all the records using the condition

RC next
This function scans each record in the table and stores the result record (record satisfying the condition)in the location pointed by  'record'.

RC closeScan
This function closes the scan operation

getRecordSize 
This function returns the record size of the schema 

Schema *createSchema 
This function creates a new schema

RC freeSchema 
This function removes a schema from memory and de-allocates all the memory space allocated to the schema.

RC createRecord 
This function creates a new record in the schema

RC attrOffset
this is a function created by us to do the assignment.
This function sets the offset (in bytes) from initial position to the specified attribute of the record into the 'result' parameter passed through the function

RC freeRecord 
This function removes the record from the memory.

RC getAttr 
This function retrieves an attribute from the given record in the specified schema

RC setAttr
This function sets the attribute value in the record
