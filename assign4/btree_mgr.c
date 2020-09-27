#include "dberror.h"
#include "btree_mgr.h"
#include "storage_mgr.h"
#include "buffer_mgr.h"
#include "tables.h"
#include <stdlib.h>
#include "dt.h"
#include "string.h"

BTreeManager *bTreeManager = NULL;

//Initiate a index manager
RC initIndexManager(void *mgmtData) {
    initStorageManager();
    return RC_OK;
}

//Close a index manager
RC shutdownIndexManager() {
    bTreeManager = NULL;
    return RC_OK;
}

//Create a B+ tree
RC createBtree(char *idxId, DataType keyType, int n) {
//malloc memory for the B+ tree
    BM_BufferPool *bm_bufferPool = (BM_BufferPool *) malloc(sizeof(BM_BufferPool));
    RC result;
    SM_FileHandle fileHandler;
    char data[PAGE_SIZE];
    if ((result = createPageFile(idxId)) != RC_OK)
        return result;
    else if ((result = openPageFile(idxId, &fileHandler)) != RC_OK)
        return result;
    else if ((result = writeBlock(0, &fileHandler, data)) != RC_OK)
        return result;
    else if ((result = closePageFile(&fileHandler)) != RC_OK)
        return result;
    else {
        bTreeManager = (BTreeManager *) malloc(sizeof(BTreeManager));
        bTreeManager->bm_bufferPool = *bm_bufferPool;
        bTreeManager->node_number = 0;
        bTreeManager->order = n + 1;
        bTreeManager->node_queue = NULL;
        bTreeManager->keyType = keyType;
        bTreeManager->entry_number = 0;
        bTreeManager->root = NULL;
        return (RC_OK);
    }
}

//Open a B+ Tree
RC openBtree(BTreeHandle **tree, char *idxId) {
    *tree = (BTreeHandle *) malloc(sizeof(BTreeHandle));
    (*tree)->mgmtData = bTreeManager;
    return initBufferPool(&bTreeManager->bm_bufferPool, idxId, 1000, RS_FIFO, NULL);
}

//Close a B+ tree
RC closeBtree(BTreeHandle *tree) {
    BTreeManager *bTreeManager = (BTreeManager *) tree->mgmtData;
    markDirty(&bTreeManager->bm_bufferPool, &bTreeManager->bm_pageHandle);
    shutdownBufferPool(&bTreeManager->bm_bufferPool);
    free(bTreeManager);
    free(tree);
    return RC_OK;
}

//Delete a B+ tree
RC deleteBtree(char *idxId) {
    return destroyPageFile(idxId);
}

//Insert a key into a B+ tree
RC insertKey(BTreeHandle *tree, Value *key, RID rid) {
    BTreeManager *bTreeManager = (BTreeManager *) tree->mgmtData;
    RID *record = (RID *) malloc(sizeof(RID));
    if (record == NULL) {
        exit(RC_INSERT_ERROR);
    } else {
        record->page = rid.page;
        record->slot = rid.slot;
    }
    RID *record_pointer = record;
    //root existed
    if (bTreeManager->root != NULL) {
        Node *leaf_pointer = findLeaf(bTreeManager->root, key);
        if (leaf_pointer->key_number >= bTreeManager->order - 1) {
            //need split
            bTreeManager->root = insertIntoLeafAfterSplitting(bTreeManager, leaf_pointer, key, record_pointer);
        } else {
            int insertion_position = 0;
            while (insertion_position < leaf_pointer->key_number) {
                if (leaf_pointer->key_values[insertion_position]->v.intV < key->v.intV) {
                    insertion_position++;
                } else {
                    break;
                }
            }
            int i = leaf_pointer->key_number - 1;
            while (i >= insertion_position) {
                leaf_pointer->node_pointers[i + 1] = leaf_pointer->node_pointers[i];
                leaf_pointer->key_values[i + 1] = leaf_pointer->key_values[i];
                i--;
            }
            bTreeManager->entry_number++;
            leaf_pointer->node_pointers[i + 1] = record_pointer;
            leaf_pointer->key_number++;
            leaf_pointer->key_values[i + 1] = key;
        }
        //printTree(tree);
        return RC_OK;
    } else {
        //no root
        Node *root = createNode(bTreeManager);
        root->node_pointers[0] = record_pointer;
        root->key_values[0] = key;
        root->node_pointers[bTreeManager->order - 1] = NULL;
        root->leaf_flag = true;
        root->parent = NULL;
        root->key_number++;
        bTreeManager->root = root;
        bTreeManager->entry_number++;
        //printTree(tree);
        return RC_OK;
    }
}

//Find a key in a B+ tree
extern RC findKey(BTreeHandle *tree, Value *key, RID *result) {
    BTreeManager *bTreeManager = (BTreeManager *) tree->mgmtData;
    RID *record;
    int i = 0;
    Node *c = findLeaf(bTreeManager->root, key);
    if (c == NULL)
        return NULL;
    for (i = 0; i < c->key_number; i++) {
        if (c->key_values[i]->v.boolV == key->v.boolV)
            break;
    }
    if (i == c->key_number)
        record = NULL;
    else
        record = (RID *) c->node_pointers[i];
//key found
    if (record != NULL) {
        *result = *record;
        return RC_OK;
    } else {
//key not found
        return RC_IM_KEY_NOT_FOUND;
    }
}

//Get the number of the nodes of a B+ tree
RC getNumNodes(BTreeHandle *tree, int *result) {
    BTreeManager *bTreeManager = (BTreeManager *) tree->mgmtData;
    *result = bTreeManager->node_number;
    return RC_OK;
}

//Get the number of the entries of a B+ tree
RC getNumEntries(BTreeHandle *tree, int *result) {
    BTreeManager *bTreeManager = (BTreeManager *) tree->mgmtData;
    *result = bTreeManager->entry_number;
    return RC_OK;
}

//Get the type of a key
RC getKeyType(BTreeHandle *tree, DataType *result) {
    BTreeManager *bTreeManager = (BTreeManager *) tree->mgmtData;
    *result = bTreeManager->keyType;
    return RC_OK;
}

//Delete a key in a B+ tree
RC deleteKey(BTreeHandle *tree, Value *key) {
    BTreeManager *bTreeManager = (BTreeManager *) tree->mgmtData;
    bTreeManager->root = delete(bTreeManager, key);
    return RC_OK;
}

//Open a scanner for a B+ tree
RC openTreeScan(BTreeHandle *tree, BT_ScanHandle **handle) {
    BTreeManager *bTreeManager = (BTreeManager *) tree->mgmtData;
    ScanManager *scanManager = malloc(sizeof(ScanManager));
    *handle = malloc(sizeof(BT_ScanHandle));
    Node *temp = bTreeManager->root;
    //tree exist and scan it
    if (bTreeManager->root != NULL) {
        for (; !temp->leaf_flag; temp = temp->node_pointers[0]);
        scanManager->key_number = temp->key_number;
        scanManager->index = 0;
        scanManager->node = temp;
        scanManager->order = bTreeManager->order;
        (*handle)->mgmtData = scanManager;
    } else {
        //no tree to scan
        return RC_NO_RECORDS_TO_SCAN;
    }
    return RC_OK;
}

//See if there are any more entries in the B+ tree
RC nextEntry(BT_ScanHandle *handle, RID *result) {
    ScanManager *scanManager = (ScanManager *) handle->mgmtData;
    int totalKeys = scanManager->key_number;
    int keyIndex = scanManager->index;
    int bTreeOrder = scanManager->order;
    Node *temp = scanManager->node;
    if (temp != NULL) {

        RID rid;

        if (keyIndex >= totalKeys) {
                  //all entries occupied
            if (temp->node_pointers[bTreeOrder - 1] == NULL) {
                return RC_IM_NO_MORE_ENTRIES;
            } else {
                //entries existed
                scanManager->index = 1;
                temp = temp->node_pointers[bTreeOrder - 1];
                scanManager->node = temp;
                scanManager->key_number = temp->key_number;
                rid = *((RID *) temp->node_pointers[0]);
            }
        } else {
            rid = *((RID *) temp->node_pointers[keyIndex]);
            scanManager->index++;
        }
        *result = rid;
        return RC_OK;
    } else {
        return RC_IM_NO_MORE_ENTRIES;
    }
}

//Close the scanner for the B+ tree
extern RC closeTreeScan(BT_ScanHandle *handle) {
    handle->mgmtData = NULL;
    free(handle);
    return RC_OK;
}


/*Inserts a new key and pointer to a node into a node, causing the node's size to
exceed the order, and causing the node to split into two.
*/
Node *insertIntoLeafAfterSplitting(BTreeManager *treeManager, Node *leaf, Value *key, RID *pointer) {
    Node *new_leaf;
    Value **temp_keys;
    void **temp_pointers;
    int insertion_index, split, new_key, i, j;
    new_leaf = createNode(treeManager);
    new_leaf->leaf_flag = true;
    int bTreeOrder = treeManager->order;
    insertion_index = 0;
    //get the order for the insertion
    while (insertion_index < bTreeOrder - 1) {
        if (leaf->key_values[insertion_index]->v.intV < key->v.intV) {
            insertion_index++;
        } else {
            break;
        }
    }
    //malloc memory for the key and the pointer to be inserted
    temp_keys = malloc(bTreeOrder * sizeof(Value));
    temp_pointers = malloc(bTreeOrder * sizeof(void *));
    int count = leaf->key_number;
    for (i = 0, j = 0; i < count; i++, j++) {
        if (j != insertion_index) {
            temp_keys[j] = leaf->key_values[i];
            temp_pointers[j] = leaf->node_pointers[i];
        } else {
            temp_keys[insertion_index] = key;
            temp_pointers[insertion_index] = pointer;
            j = j + 1;
            temp_keys[j] = leaf->key_values[i];
            temp_pointers[j] = leaf->node_pointers[i];
        }

    }

    temp_keys[insertion_index] = key;
    temp_pointers[insertion_index] = pointer;

    leaf->key_number = 0;
    split = (bTreeOrder - 1) / 2 + 1;
    for (j = 0; j < bTreeOrder - split; j++) {
        new_leaf->node_pointers[j] = temp_pointers[j + split];
        new_leaf->key_values[j] = temp_keys[j + split];
    }
    new_leaf->key_number = bTreeOrder - split;
    for (i = 0; i < split; i++) {
        leaf->node_pointers[i] = temp_pointers[i];
        leaf->key_values[i] = temp_keys[i];
    }

    leaf->key_number = split;
    free(temp_pointers);
    free(temp_keys);


    void **tmp_pointer = leaf->node_pointers[bTreeOrder - 1];
    leaf->node_pointers[bTreeOrder - 1] = new_leaf;
    new_leaf->node_pointers[bTreeOrder - 1] = tmp_pointer;
    i = leaf->key_number;
    int order = bTreeOrder - 1;
    for (i; i < order; i++) {
        leaf->key_values[i] = NULL;
    }
    i = new_leaf->key_number;
    for (i; i < order; i++) {
        new_leaf->node_pointers[i] = NULL;
    }


    new_leaf->parent = leaf->parent;
    new_key = new_leaf->key_values[0];
    treeManager->entry_number++;
    return insertIntoParent(treeManager, leaf, new_key, new_leaf);
}

//Inserts a new node (leaf or internal node) into the B+ tree.
Node *insertIntoParent(BTreeManager *treeManager, Node *left, Value *key, Node *right) {

    int left_index, i, tmp;
    Node *parent = left->parent;
    int bTreeOrder = treeManager->order;
//no root
    if (parent == NULL) {
        Node *root = createNode(treeManager);
        root->key_values[0] = key;
        root->node_pointers[1] = right;
        root->parent = NULL;
        root->node_pointers[0] = left;
        root->key_number++;
        left->parent = root;
        right->parent = root;
        return root;
    }

// If it's a leaf node, find the parent's pointer to the left node.
    left_index = 0;
    while (left_index <= parent->key_number && parent->node_pointers[left_index] != left)
        left_index++;

// No split needed

    tmp = bTreeOrder - 1;
    if (parent->key_number >= tmp) {

        int i, j, split, k_prime, tmp;
        Node *new_node;
        Value **temp_keys;
        Node **temp_pointers;

        tmp = treeManager->order + 1;
        temp_pointers = malloc((tmp) * sizeof(Node *));

        temp_keys = malloc(treeManager->order * sizeof(Value *));


        tmp = parent->key_number + 1;
        j = 0;
        for (i = 0; i < tmp; i++) {
            if (j != left_index + 1) {
                temp_pointers[j] = parent->node_pointers[i];
            } else {
                j = i + 1;
                temp_pointers[j] = parent->node_pointers[i];
            }
            j++;
        }
        temp_pointers[left_index + 1] = right;
        j = 0;
        for (i = 0; i < parent->key_number; i++) {

            if (j != left_index) {
                temp_keys[j] = parent->key_values[i];
            } else {
                j = i + 1;
                temp_keys[j] = parent->key_values[i];
            }
            j = j + 1;
        }
        split = (treeManager->order - 1) / 2 + 1;
        if ((treeManager->order - 1) % 2 != 0)
            split = split + 1;
        temp_keys[left_index] = key;

        new_node = createNode(treeManager);
        parent->key_number = 0;
        tmp = split - 1;
        for (i = 0; i < tmp; i++) {
            parent->key_number++;
            parent->node_pointers[i] = temp_pointers[i];
            parent->key_values[i] = temp_keys[i];

        }
        i++;
        k_prime = temp_keys[tmp];
        j = 0;
        for (i; i < treeManager->order; i++) {
            new_node->node_pointers[j] = temp_pointers[i];
            new_node->key_values[j] = temp_keys[i];
            new_node->key_number++;
            j++;
        }
        new_node->parent = parent->parent;
        new_node->node_pointers[treeManager->order - split] = temp_pointers[treeManager->order];
        free(temp_pointers);
        free(temp_keys);
        parent->node_pointers[tmp] = temp_pointers[tmp];
// In case it cannot accomodate, then split the node preserving the B+ Tree properties.
        return insertIntoParent(treeManager, parent, k_prime, new_node);
    }
    for (i = parent->key_number; i > left_index; i--) {
        parent->node_pointers[i + 1] = parent->node_pointers[i];
        parent->key_values[i] = parent->key_values[i - 1];
    }

    parent->node_pointers[left_index + 1] = right;
    parent->key_values[left_index] = key;
    parent->key_number++;

    return treeManager->root;

}

//Creates a new general node, which can be adapted to serve as either a leaf or an internal node.
Node *createNode(BTreeManager *treeManager) {
    treeManager->node_number++;
    int bTreeOrder = treeManager->order;
//null new node
    Node *new_node = malloc(sizeof(Node));
    if (new_node == NULL) {
        perror("Node creation.");
        exit(RC_INSERT_ERROR);
    }
//no value for new node
    new_node->key_values = malloc((bTreeOrder - 1) * sizeof(Value *));
    if (new_node->key_values == NULL) {
        perror("New node keys array.");
        exit(RC_INSERT_ERROR);
    }
//no pointer for new node
    new_node->node_pointers = malloc(bTreeOrder * sizeof(void *));
    if (new_node->node_pointers == NULL) {
        perror("New node pointers array.");
        exit(RC_INSERT_ERROR);
    }
//create the new node
    new_node->leaf_flag = false;
    new_node->key_number = 0;
    new_node->parent = NULL;
    new_node->next = NULL;
    return new_node;
}

/*Searches for the key from root to the leaf.
Returns the leaf containing the given key.
*/
Node *findLeaf(Node *root, Value *key) {
    int i = 0;
    Node *c = root;
    if (c == NULL) {
        return c;
    }
    while (!c->leaf_flag) {
        i = 0;
        while (i < c->key_number) {
            if ((key->v.intV >= c->key_values[i]->v.intV) || key->v.boolV == c->key_values[i]->v.boolV) {
                i++;
            } else
                break;
        }
        c = (Node *) c->node_pointers[i];
    }
    return c;
}

/*Deletes an entry from the B+ tree. It removes the record having the specified key and pointer
from the leaf, and then makes all appropriate changes to preserve the B+ tree properties.
*/
Node *deleteEntry(BTreeManager *treeManager, Node *n, Value *key, void *pointer) {
    int min_keys;
    Node *neighbor, *new_root;
    int neighbor_index;
    int k_prime_index, k_prime;
    int capacity;
    int bTreeOrder = treeManager->order;


    int i, num_pointers, tmp;
    i = 0;

    while (n->key_values[i]->v.boolV != key->v.boolV)
        i++;
    tmp = n->key_number - 1;
    n->key_number--;
    treeManager->entry_number--;
    for (i; i < tmp; i++)
        n->key_values[i] = n->key_values[i + 1];

    if (!n->leaf_flag) {
        num_pointers = tmp + 2;
    } else {
        num_pointers = tmp + 1;
    }


    for (i = 0; n->node_pointers[i] != pointer; i++);

    for (i; i < num_pointers - 1; i++)
        n->node_pointers[i] = n->node_pointers[i + 1];

    if (!n->leaf_flag) {
        i = n->key_number + 1;
        for (i; i < treeManager->order; i++)
            n->node_pointers[i] = NULL;
    } else {
        tmp = treeManager->order - 1;
        for (i = n->key_number; i < tmp; i++)
            n->node_pointers[i] = NULL;
    }

    // If n is root then adjust it
    if (n == treeManager->root) {
        if (treeManager->root->key_number > 0)
            return treeManager->root;

        if (!treeManager->root->leaf_flag) {
            new_root = NULL;
        } else {
            new_root = treeManager->root->node_pointers[0];
            new_root->parent = NULL;
        }
        free(treeManager->root->key_values);
        free(treeManager->root->node_pointers);
        free(treeManager->root);
        return new_root;
    }
//if n is a leaf node
    if (n->leaf_flag) {
        if ((bTreeOrder - 1) % 2 == 0)
            min_keys = (bTreeOrder - 1) / 2;
        else
            min_keys = (bTreeOrder - 1) / 2 + 1;
    } else {
        if ((bTreeOrder) % 2 == 0)
            min_keys = (bTreeOrder) / 2;
        else
            min_keys = (bTreeOrder) / 2 + 1;
        min_keys--;
    }
    // Node stays at or above minimum.
    if (n->key_number >= min_keys)
        return treeManager->root;
    // If the node falls below minimum, either merging or redistribution is needed.
    // Find the appropriate neighbor node with which to merge. Also find the key (k_prime)
    // in the parent between the pointer to node n and the pointer to the neighbor.
    neighbor_index = -100;

    for (i = 0; i <= n->parent->key_number; i++)
        if (n->parent->node_pointers[i] == n)
            neighbor_index = i - 1;
    if (neighbor_index == -100) {
        printf("Search for nonexistent pointer to node in parent.\n");
        return RC_ERROR;
    }
//merge with the neighbor
    if (neighbor_index != -1) {
        k_prime_index = neighbor_index;
    } else {
        k_prime_index = 0;
    }
    k_prime = n->parent->key_values[k_prime_index];
    //merge with parent
    if (neighbor_index != -1) {
        neighbor = n->parent->node_pointers[neighbor_index];
    } else {
        neighbor = n->parent->node_pointers[1];
    }
    //check the capacity for the node
    if (!n->leaf_flag) {
        capacity = bTreeOrder - 1;
    } else {
        capacity = bTreeOrder;
    }
    if (neighbor->key_number + n->key_number >= capacity) {

        if (neighbor_index == -1) {
            if (!n->leaf_flag) {
                n->node_pointers[n->key_number + 1] = neighbor->node_pointers[0];
                n->key_values[n->key_number] = k_prime;
                Node *tmp = (Node *) n->node_pointers[n->key_number + 1];
                tmp->parent = n;
                n->parent->key_values[k_prime_index] = neighbor->key_values[0];
            } else {
                n->node_pointers[n->key_number] = neighbor->node_pointers[0];
                n->key_values[n->key_number] = neighbor->key_values[0];
                n->parent->key_values[k_prime_index] = neighbor->key_values[1];
            }
            int i = 0;
            while (i < neighbor->key_number - 1) {
                neighbor->key_values[i] = neighbor->key_values[i + 1];
                neighbor->node_pointers[i] = neighbor->node_pointers[i + 1];
                i++;
            }
            if (!n->leaf_flag)
                neighbor->node_pointers[i] = neighbor->node_pointers[i + 1];

        } else {
            if (!n->leaf_flag)
                n->node_pointers[n->key_number + 1] = n->node_pointers[n->key_number];
            int i = n->key_number;
            while (i > 0) {
                n->key_values[i] = n->key_values[i - 1];
                n->node_pointers[i] = n->node_pointers[i - 1];
                i--;
            }
            if (n->leaf_flag) {
                n->node_pointers[0] = neighbor->node_pointers[neighbor->key_number - 1];
                neighbor->node_pointers[neighbor->key_number - 1] = NULL;
                n->key_values[0] = neighbor->key_values[neighbor->key_number - 1];
                n->parent->key_values[k_prime_index] = n->key_values[0];
            } else {
                n->node_pointers[0] = neighbor->node_pointers[neighbor->key_number];
                Node *tmp = (Node *) n->node_pointers[0];
                tmp->parent = n;
                neighbor->node_pointers[neighbor->key_number] = NULL;
                n->key_values[0] = k_prime;
                n->parent->key_values[k_prime_index] = neighbor->key_values[neighbor->key_number - 1];
            }
        }

        neighbor->key_number--;
        n->key_number++;
        return treeManager->root;

    } else {

        int i, j, neighbor_insertion_index, n_end;
        Node *tmp;
        int bTreeOrder = treeManager->order;
// Swap neighbor with node if node is on the extreme left and neighbor is to its right.
        if (neighbor_index == -1) {
            tmp = n;
            n = neighbor;
            neighbor = tmp;
        }
    // Starting point in the neighbor for copying keys and pointers from n.
    // n and neighbor have swapped places in the special case of n being a leftmost child.
        neighbor_insertion_index = neighbor->key_number;
    // If its a non-leaf node, append k_prime and the following pointer.
    // Also, append all pointers and keys from the neighbor.
        if (!n->leaf_flag) {
            neighbor->key_values[neighbor_insertion_index] = k_prime;
            neighbor->key_number++;

            n_end = n->key_number;

            for (i = neighbor_insertion_index + 1, j = 0; j < n_end; i++, j++) {
                neighbor->key_values[i] = n->key_values[j];
                neighbor->node_pointers[i] = n->node_pointers[j];
                neighbor->key_number++;
                n->key_number--;
            }

            neighbor->node_pointers[i] = n->node_pointers[j];
 // Link all children to the same parent.
            for (i = 0; i < neighbor->key_number + 1; i++) {
                tmp = (Node *) neighbor->node_pointers[i];
                tmp->parent = neighbor;
            }
        } else {
                    // Link all the pointers in the leaf nodes to their neighbors
        // Set the neighbor's last pointer to point to what had been n's right neighbor.
            for (i = neighbor_insertion_index, j = 0; j < n->key_number; i++, j++) {
                neighbor->key_values[i] = n->key_values[j];
                neighbor->node_pointers[i] = n->node_pointers[j];
                neighbor->key_number++;
            }
            neighbor->node_pointers[bTreeOrder - 1] = n->node_pointers[bTreeOrder - 1];
        }

        treeManager->root = deleteEntry(treeManager, n->parent, k_prime, n);

        free(n->key_values);
        free(n->node_pointers);
        free(n);
        return treeManager->root;

    }
}

//Deletes the the entry/record having the specified key.
Node *delete(BTreeManager *treeManager, Value *key) {
    Node *record_node;
    Node *leaf_node;
    int position = 0;
    leaf_node = findLeaf(treeManager->root, key);
    while (position < leaf_node->key_number) {
        if (leaf_node->key_values[position]->v.boolV == key->v.boolV)
            break;
        position++;
    }
    (position != leaf_node->key_number) ? (record_node = (RID *) leaf_node->node_pointers[position])
                                        : (record_node = NULL);
    (RID *) leaf_node;
    if (record_node != NULL && leaf_node != NULL) {
        treeManager->root = deleteEntry(treeManager, leaf_node, key, record_node);
        free(record_node);
    }
    return treeManager->root;
}

//Helps in printing the B+ Tree,enqueue the nodes into a queue
void enqueue(BTreeManager *treeManager, Node *new_node) {

    Node * c;
    if (treeManager->node_queue == NULL) {
        treeManager->node_queue = new_node;
        treeManager->node_queue->next = NULL;
    } else {
        c = treeManager->node_queue;
        while (c->next != NULL) {
            c = c->next;
        }
        c->next = new_node;
        new_node->next = NULL;
    }
}

//Helps in printing the B+ Tree,dequeue the nodes from a queue
Node *dequeue(BTreeManager *treeManager) {
    Node *node = treeManager->node_queue;
    treeManager->node_queue = treeManager->node_queue->next;
    node->next = NULL;
    return node;
}

//Print all the nodes of a B+ tree
extern char *printTree(BTreeHandle *tree) {
    BTreeManager *treeManager = (BTreeManager *) tree->mgmtData;
    printf("\nPRINTING TREE:\n");
    if(treeManager->root == NULL){
        printf("Empty tree.\n");
        return '\0';
    }else {
        Node *n = NULL;
        int i = 0;
        treeManager->node_queue = NULL;
        enqueue(treeManager, treeManager->root);
        while (TRUE) {
            if (treeManager->node_queue != NULL) {
                n = dequeue(treeManager);
                if (n->parent != NULL)
                {
                    if(n == n->parent->node_pointers[0]) {
                        int length = 0;
                        Node *c = n;
                        while (c != treeManager->root) {
                            c = c->parent;
                            length++;
                        }
                        if (length != 0) {
                            printf("\n");
                        }
                    }
                }

                // Print key depending on datatype of the key.
                int i = 0;
                while (i < n->key_number) {
                    if (bTreeManager->keyType == DT_FLOAT) {
                        printf("%.02f ", (*n->key_values[i]).v.floatV);
                    } else if (bTreeManager->keyType == DT_INT) {
                        printf("%d ", (*n->key_values[i]).v.intV);
                    } else if (bTreeManager->keyType == DT_BOOL) {
                        printf("%d ", (*n->key_values[i]).v.boolV);
                    } else if (bTreeManager->keyType == DT_STRING) {
                        printf("%s ", (*n->key_values[i]).v.stringV);
                    }
                    printf("(%d - %d) ", ((RID *) n->node_pointers[i])->page, ((RID *) n->node_pointers[i])->slot);
                    i++;
                }
                if (!n->leaf_flag)
                    for (i = 0; i < n->key_number+1; i++)
                        enqueue(treeManager, n->node_pointers[i]);

                printf("| ");
            }else{
                break;
            }
        }
        printf("\n");

        return '\0';
    }
}


