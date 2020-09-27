#ifndef DBERROR_H
#define DBERROR_H

#include "stdio.h"

/* module wide constants */
#define PAGE_SIZE 4096

/* return code definitions */
typedef int RC;

#define RC_OK 0
#define RC_FILE_NOT_FOUND 1
#define RC_WRITE_FAILED 3
#define RC_READ_NON_EXISTING_PAGE 4
#define RC_ERROR 400
#define RC_PINNED_PAGES_IN_BUFFER 500
#define RC_RM_COMPARE_VALUE_OF_DIFFERENT_DATATYPE 200
#define RC_RM_BOOLEAN_EXPR_ARG_IS_NOT_BOOLEAN 202
#define RC_INSERT_ERROR 702
#define RC_RM_NO_MORE_TUPLES 203
#define RC_IM_KEY_NOT_FOUND 300
#define RC_NO_RECORDS_TO_SCAN 703
#define RC_IM_NO_MORE_ENTRIES 303


/* holder for error messages */
extern char *RC_message;

/* print a message to standard out describing the error */
extern void printError(RC error);

extern char *errorMessage(RC error);

#define THROW(rc, message) \
  do {              \
    RC_message=message;      \
    return rc;          \
  } while (0)          \

// check the return code and exit if it is an error
#define CHECK(code)                            \
  do {                                    \
    int rc_internal = (code);                        \
    if (rc_internal != RC_OK)                        \
      {                                    \
    char *message = errorMessage(rc_internal);            \
    printf("[%s-L%i-%s] ERROR: Operation returned error: %s\n",__FILE__, __LINE__, __TIME__, message); \
    free(message);                            \
    exit(1);                            \
      }                                    \
  } while(0);


#endif
