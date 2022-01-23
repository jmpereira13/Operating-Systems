#ifndef FS_H
#define FS_H
#include "state.h"
#include <stdbool.h>

/* intializes the file system and creates root node = 0 */
void init_fs();
/* destroy the file system and its content */
void destroy_fs();
/* check if one directory is empty, returns succes or fail */
int is_dir_empty(DirEntry *dirEntries);
/* moves a file/directory that was in a certain path to a knew one */
int move(char *pathnameOld, char *pathnameNew, int *lock_array, int *array_head, int specific_inumber);
/* create's a new node given a path. return success or fail */ 
int create(char *name, type nodeType, int *lock_array, int *array_head, int specificInumber);
/* delete a node given a path, return success or fail */
int delete(char *name, int *lock_array, int *array_head, bool state);
/* looks up for a node given a path, return the id number of the node */
int lookup(char *name, int *lock_array, int *array_head);
/* prints the tree in the output file */
void print_tecnicofs_tree(FILE *fp);
/* prints the exact time taken to run the program */
void printTimer( struct timeval current_time, struct timeval final_time );
/* blocks all "writing" threads and prints tecnicofs tree */
void block_printfs(FILE *fp);


#endif /* FS_H */
