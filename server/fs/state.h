#ifndef INODES_H
#define INODES_H

#include <stdio.h>
#include <stdlib.h>
#include "../tecnicofs-api-constants.h"

/* FS root inode number */
#define FS_ROOT 0

#define FREE_INODE -1
#define INODE_TABLE_SIZE 50
#define MAX_DIR_ENTRIES 20

#define SUCCESS 0
#define FAIL -1

#define DELAY 5000


/*
 * Contains the name of the entry and respective i-number
 */
typedef struct dirEntry {
	char name[MAX_FILE_NAME];
	int inumber;
} DirEntry;

/*
 * Data is either text (file) or entries (DirEntry)
 */
union Data {
	char *fileContents; /* for files */
	DirEntry *dirEntries; /* for directories */
};

/*
 * I-node definition
 */
typedef struct inode_t {
	type nodeType;
	union Data data;

	/* OUR */
	pthread_rwlock_t inodeLock; /* personal lock */
    /* more i-node attributes will be added in future exercises */
} inode_t;

/**/
void insert_delay(int cycles);
/* init the table of inodes */
void inode_table_init();
/* destroy the table of inodes*/
void inode_table_destroy();
/* create an inode */
int inode_create(type nType);
/* delete the inode with given inumber*/
int inode_delete(int inumber);
/* get the type and Data of a inode with a given inumber*/
int inode_get(int inumber, type *nType, union Data *data);
/* set the fileContents of a inode*/
int inode_set_file(int inumber, char *fileContents, int len);
/* retira um sub no da tabela de entradas de um no pai(diretoria) */
int dir_reset_entry(int inumber, int sub_inumber);
/* adiciona um sub no na tabela de entradas de um no pai(diretoria) */
int dir_add_entry(int inumber, int sub_inumber, char *sub_name);
/* print tree of the inode table*/
void inode_print_tree(FILE *fp, int inumber, char *name);
/* lock the inode with a given idNumber */
void lockInode(int idNumber, char *type);
/* unlock the inode with a given idNumber */
void unlockInode( int idNumber );
/* free the inodes */
void freeLock(int *array, int *head);
/* unlocks all inodes of the array */
void unlockArray(int *array, int *head);
/* init the lock from a given inumber */
void init_lock(int inumber);


#endif /* INODES_H */
