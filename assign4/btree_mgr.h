#ifndef BTREE_MGR_H
#define BTREE_MGR_H


#include "buffer_mgr.h"
#include "dberror.h"
#include "tables.h"

// structure for accessing btrees
typedef struct BTreeHandle {
    DataType keyType;
    char *idxId;
    void *mgmtData;
} BTreeHandle;

typedef struct BT_ScanHandle {
    BTreeHandle *tree;
    void *mgmtData;
} BT_ScanHandle;
///###
typedef struct Node {
    void ** node_pointers;
    int key_number;
    Value ** key_values;
    struct Node * parent;
    bool leaf_flag;
    struct Node * next;
} Node;

typedef struct BTreeManager {
    BM_BufferPool bm_bufferPool;
    int entry_number;
    int order;
    Node * root;
    int node_number;
    Node * node_queue;
    DataType keyType;
    BM_PageHandle bm_pageHandle;
} BTreeManager;

typedef struct ScanManager {
    Node * node;
    int index;
    int key_number;
    int order;
} ScanManager;

///###

// init and shutdown index manager
extern RC initIndexManager (void *mgmtData);
extern RC shutdownIndexManager ();

// create, destroy, open, and close an btree index
extern RC createBtree (char *idxId, DataType keyType, int n);
extern RC openBtree (BTreeHandle **tree, char *idxId);
extern RC closeBtree (BTreeHandle *tree);
extern RC deleteBtree (char *idxId);

// access information about a b-tree
extern RC getNumNodes (BTreeHandle *tree, int *result);
extern RC getNumEntries (BTreeHandle *tree, int *result);
extern RC getKeyType (BTreeHandle *tree, DataType *result);

// index access
extern RC findKey (BTreeHandle *tree, Value *key, RID *result);
extern RC insertKey (BTreeHandle *tree, Value *key, RID rid);
extern RC deleteKey (BTreeHandle *tree, Value *key);
extern RC openTreeScan (BTreeHandle *tree, BT_ScanHandle **handle);
extern RC nextEntry (BT_ScanHandle *handle, RID *result);
extern RC closeTreeScan (BT_ScanHandle *handle);

// debug and test functions
extern char *printTree (BTreeHandle *tree);
void enqueue(BTreeManager * treeManager, Node * new_node);
Node * dequeue(BTreeManager * treeManager);


Node * findLeaf(Node * root, Value * key);
Node * createNode(BTreeManager * treeManager);
Node * insertIntoLeafAfterSplitting(BTreeManager * treeManager, Node * leaf, Value * key, RID * pointer);
Node * insertIntoParent(BTreeManager * treeManager, Node * left, Value * key, Node * right);
Node * deleteEntry(BTreeManager * treeManager, Node * n, Value * key, void * pointer);
Node * delete(BTreeManager * treeManager, Value * key);

#endif // BTREE_MGR_H
