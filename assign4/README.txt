Project description:
Our project used C language to simulate a basic database system. For this assignment 4,we implemented a B+-tree index for the system.  

Prerequisites:
Linux base system 
GCC command 
Make command
Clion
Cmakelist

Running the tests:
Use Clion and Cmakelist to build and run the program



Function:
RC initIndexManager
Initiate a index manager

RC shutdownIndexManager
Shut down the index manager

RC createBtree
Create a B+ Tree with given parameters 

RC openBtree
Open a B+ Tree 

RC closeBtree
Close a B+ tree

RC deleteBtree
Delete a B+ tree

RC insertKey
Insert a key into a B+ tree

RC findKey
Find a key in a B+ tree

RC getNumNodes
Get the number of the nodes of a B+ tree

RC getNumEntries
Get the number of the entries of a B+ tree

RC getKeyType
Get the type of a key

RC deleteKey
Delete a key in a B+ tree

RC openTreeScan
Open a scanner for a B+ tree

RC nextEntry
See if there are any more entries in the B+ tree

RC closeTreeScan
Close the scanner for the B+ tree

Node *insertIntoLeafAfterSplitting
Inserts a new key and pointer to a node into a node, causing the node's size to 
exceed the order, and causing the node to split into two.

Node *insertIntoParent
Inserts a new node (leaf or internal node) into the B+ tree.

Node *createNode
Creates a new general node, which can be adapted to serve as either a leaf or an internal node.

Node *findLeaf
Searches for the key from root to the leaf.
Returns the leaf containing the given key.

Node *deleteEntry
Deletes an entry from the B+ tree. It removes the record having the specified key and pointer
from the leaf, and then makes all appropriate changes to preserve the B+ tree properties.

Node *delete
Deletes the the entry/record having the specified key.

void enqueue
Helps in printing the B+ Tree,enqueue the nodes into a queue

Node *dequeue
Helps in printing the B+ Tree,dequeue the nodes from a queue

char *printTree
Print all the nodes of a B+ tree