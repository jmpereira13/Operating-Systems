#include "operations.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <sys/time.h>
#include <unistd.h>


/* Given a path, fills pointers with strings for the parent path and child
 * file name
 * Input:
 *  - path: the path to split. ATENTION: the function may alter this parameter
 *  - parent: reference to a char*, to store parent path
 *  - child: reference to a char*, to store child file name
 */
void split_parent_child_from_path(char * path, char ** parent, char ** child) {

	int n_slashes = 0, last_slash_location = 0;
	int len = strlen(path);

	// deal with trailing slash ( a/x vs a/x/ )
	// in case of ( a/x/ )
	if (path[len-1] == '/') {
		path[len-1] = '\0';
	}

	// go through the enteire string
	for (int i=0; i < len; ++i) {
		// in case of non last slash
		if (path[i] == '/' && path[i+1] != '\0') {
			last_slash_location = i;
			n_slashes++;
		}
	}

	// in case of no slashes thous no parent
	if (n_slashes == 0) { // root directory
		*parent = "";
		*child = path;
		return;
	}

	// normal cases
	path[last_slash_location] = '\0';
	/* parent has all the words before child */
	*parent = path;
	/*child has the pointer for the last word */
	*child = path + last_slash_location + 1;

}


/*
 * Initializes tecnicofs and creates root node.
 */
void init_fs() {
	// putting every space at NULL
	inode_table_init();

	/* create root inode and returns the inumber */
	int root = inode_create(T_DIRECTORY);

	//if root is diferrent than 0 than there is an error
	if (root != FS_ROOT) {
		printf("failed to create node for tecnicofs root\n");
		exit(EXIT_FAILURE);
	}
}


/*
 * Destroy tecnicofs and inode table.
 */
void destroy_fs() {
	inode_table_destroy();
}


/*
 * Checks if content of directory is not empty.
 * Input:
 *  - entries: entries of directory
 * Returns: SUCCESS or FAIL
 */

int is_dir_empty(DirEntry *dirEntries) {

	// if such directory doesn't exist
	if (dirEntries == NULL) {
		return FAIL;
	}

	// check to see if there is one occupied
	for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
		if (dirEntries[i].inumber != FREE_INODE) {
			return FAIL;
		}
	}

	// everything checks out
	return SUCCESS;
}


/*
 * Looks for node in directory entry from name.
 * Input:
 *  - name: path of node
 *  - entries: entries of directory
 * Returns:
 *  - inumber: found node's inumber
 *  - FAIL: if not found
 */
int lookup_sub_node(char *name, DirEntry *entries) {

	// if such entrie doesnt exist
	if (entries == NULL) {
		return FAIL;
	}

	// check if there is any node occupied and with that name
	for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
        if (entries[i].inumber != FREE_INODE && strcmp(entries[i].name, name) == 0) {
            return entries[i].inumber;
        }
    }

	// couldn't find the entrie
	return FAIL;
}


/**
 * Function move
 * Input:
 * 	- pahtnameOld: path before moving
 *  - pathnameNew: path targe(after moving)
 *	- lock_array: array of the inode locks
 * 	- head:	head of the lock_array
 *  - specific_inumber: previous inumber
 * Returns: SUCCESS or FAILS
 */
int move(char *pathnameOld, char *pathnameNew, int *lock_array, int *array_head, int specific_inumber){
	/*lock*/
	if(delete(pathnameOld, lock_array, array_head, true)==FAIL)
		/*unlock*/
		return FAIL;
	if(create(pathnameNew, 'f', lock_array, array_head, specific_inumber)==FAIL){
		printf("BURRO DEU FAIL\n");
		create(pathnameOld, 'f', lock_array, array_head, specific_inumber);
		/*unlock*/
		return FAIL;
	}
	return SUCCESS;

}


/*
 * Creates a new node given a path.
 * Input:
 *  - name: path of node
 *  - nodeType: type of node
 *  - specific Inumber: specified inumber ( -1 for non specified inumber )
 * Returns: SUCCESS or FAIL
 */
int create(char *name, type nodeType, int *lock_array, int *array_head, int specificInumber){

	int parent_inumber, child_inumber;
	char *parent_name, *child_name, name_copy[MAX_FILE_NAME];
	/* use for copy */
	type pType;
	union Data pdata;

	/* knowing the path, parent and child */
	strcpy(name_copy, name);
	split_parent_child_from_path(name_copy, &parent_name, &child_name);

	/* know the parents inumber */
	parent_inumber = lookup(parent_name, lock_array, array_head);

	/* if such parent doesn't exist */
	if (parent_inumber == FAIL) {
		printf("failed to create %s, invalid parent dir %s\n",
		        name, parent_name);
		/* unlock */
		unlockArray(lock_array, array_head);
		return FAIL;
	}

	/* know the content of such id */
	inode_get(parent_inumber, &pType, &pdata);

	/* if ptype doesn't match up */
	if(pType != T_DIRECTORY) {
		printf("failed to create %s, parent %s is not a dir\n",
		        name, parent_name);
		unlockArray(lock_array, array_head);
		return FAIL;
	}

	/* if there is already a node with such name */
	if (lookup_sub_node(child_name, pdata.dirEntries) != FAIL) {
		printf("failed to create %s, already exists in dir %s\n",
		       child_name, parent_name);
		/* unlock inode lock's */
		unlockArray(lock_array, array_head);
		return FAIL;
	}

	/* no specific inumber */
	if(specificInumber == -1)
		child_inumber = inode_create(nodeType);

	/* specific inumber */
	else if(specificInumber != -1)
		child_inumber = specificInumber;

	/* check if the child exists */
	if (child_inumber == FAIL) {
		printf("failed to create %s in  %s, couldn't allocate inode\n",
		        child_name, parent_name);
		/* unlock inode lock's */
		unlockArray(lock_array, array_head);
		return FAIL;
	}

	/*if we couldn't add an entry to parent */
	if (dir_add_entry(parent_inumber, child_inumber, child_name) == FAIL) {
		printf("could not add entry %s in dir %s\n",
		       child_name, parent_name);
		/* unlock inode lock's */
		unlockArray(lock_array, array_head);
		return FAIL;
	}
	
	/* unlock inode lock's */
	unlockArray(lock_array, array_head);
	return SUCCESS;
}


/*
 * Deletes a node given a path.
 * Input:
 *  - name: path of node
 * 	- lock_array: inode array locks
 *  - array_head: head of the array
 *  - state: true(save status) / false(dont save status)
 * Returns: SUCCESS or FAIL
 */
int delete(char *name, int *lock_array, int *array_head, bool state){

	int parent_inumber, child_inumber;
	char *parent_name, *child_name, name_copy[MAX_FILE_NAME];
	/* use for copy */
	type pType, cType;
	union Data pdata, cdata;

	strcpy(name_copy, name);
	split_parent_child_from_path(name_copy, &parent_name, &child_name);

	/* know the parents id */
	parent_inumber = lookup(parent_name, lock_array, array_head);

	/* if the parents id number doesn't checks out */
	if (parent_inumber == FAIL) {
		printf("failed to delete %s, invalid parent dir %s.\n",
		        child_name, parent_name);
		/* unlock inode lock's */
		unlockArray(lock_array, array_head);
		return FAIL;
	}

	/* know the content of the parent inode */
	inode_get(parent_inumber, &pType, &pdata);

	/* if parents type isn't a directory */
	if(pType != T_DIRECTORY) {
		printf("failed to delete %s, parent %s is not a dir.\n",
		        child_name, parent_name);
		/* unlock inode lock's */
		unlockArray(lock_array, array_head);
		return FAIL;
	}

	/* know the childs id number */
	child_inumber = lookup_sub_node(child_name, pdata.dirEntries);

	/* if the childs id number doesn't check out */
	if (child_inumber == FAIL) {
		printf("could not delete %s, does not exist in dir %s.\n",
		       name, parent_name);
		/* unlock inode lock's */
		unlockArray(lock_array, array_head);
		return FAIL;
	}

	/* know the type of node from child */
	inode_get(child_inumber, &cType, &cdata);

	/* if i'm trying to move it */ 
	if(state == false){
		if (cType == T_DIRECTORY && is_dir_empty(cdata.dirEntries) == FAIL) {
			printf("could not delete %s: is a directory and not empty.\n", name);
			/* unlock inode lock's */
			unlockArray(lock_array, array_head);
			return FAIL;
		}
	}

	/* remove entry from folder that contained deleted node */
	if (dir_reset_entry(parent_inumber, child_inumber) == FAIL) {
		printf("failed to delete %s from dir %s.\n",
		       child_name, parent_name);
		/* unlock inode lock's */
		unlockArray(lock_array, array_head);
		return FAIL;
	}

	/* if we don't need to save the inode status*/
	if(state == false){
		/* delete inumber information */
		if (inode_delete(child_inumber) == FAIL) {
			printf("could not delete inode number %d from dir %s.\n",child_inumber, parent_name);
			/* unlock inode lock's */
			unlockArray(lock_array, array_head);
			return FAIL;
		}
	}

	/* unlock inode lock's */
	unlockArray(lock_array, array_head);
	return SUCCESS;
}


/**
 * know the number of directories to lock
 * Input:
 * 	- path: path
 * Returns:
 *  - number of names in path
 */
int path_lenght(char *path){
	int n_slashes = 0;
	int len = strlen(path);
	// go through the enteire string
	for (int i=0; i < len; i++) {
		// in case of non last slash
		if (path[i] == '/' && path[i+1] != '\0') {
			n_slashes++;
		}
	}
	return n_slashes + 1;
}


/*
 * Lookup for a given path.
 * Input:
 *  - name: path of node
 * Returns:
 *  inumber: identifier of the i-node, if found
 *     FAIL: otherwise
 */
int lookup(char *name, int *lock_array, int *array_head) {
	char full_path[MAX_FILE_NAME];
	char delim[] = "/";
	char *saveptr;

	strcpy(full_path, name);
	int path_size = path_lenght(name);

	/* start at root node */
	int current_inumber = FS_ROOT;
	/* use for copy */
	type nType;
	union Data data;

	/* if the parents path is just fs_root */
	if (strcmp(full_path, "") == 0){
		lockInode(current_inumber, "W");
		lock_array[*array_head] = current_inumber;
	} else {
		lockInode(current_inumber, "R");
		lock_array[*array_head] = current_inumber;
	}
	/* get root inode data and type */
	inode_get(current_inumber, &nType, &data);

	/* stripout the delim */
	char *path = strtok_r(full_path, delim, &saveptr);

	/* search for all sub nodes */
	while (path != NULL && (current_inumber = lookup_sub_node(path, data.dirEntries)) != FAIL) {
		if(current_inumber != FAIL){
			/* if there is still more than 1 inode left in path*/
			if (path_size > 1){
				/* lock for read and add to inode array lock */
				lockInode( current_inumber, "R");
				lock_array[++(*array_head)] = current_inumber;

			/* if there is only one more inode to lock*/
			} else if (path_size == 1){
				/* lock for write and add to inode array lock */
				lockInode( current_inumber, "W");
				lock_array[++(*array_head)] = current_inumber;
			}

		/*error*/
		} else{ unlockArray(lock_array, array_head);}

		/* get the information of the inode and pass to the next inode */
		inode_get(current_inumber, &nType, &data);
		path = strtok_r(NULL, delim, &saveptr);
		path_size--;
	}
	/* return last inumber */
	return current_inumber;
}


/*
 * Prints the amount of time taken to run the program
 * Input:
 *  - current_time: initial time
 *  - final_time: time of ending
 */
void printTimer( struct timeval current_time, struct timeval final_time ){
	double diff;
	/* ending time */
	gettimeofday( &final_time, NULL );
	diff = final_time.tv_sec - current_time.tv_sec + (final_time.tv_usec - current_time.tv_usec)/1000000.0;
	printf("TecnicoFS completed in %.4f seconds.\n", diff);
}


/*
 * Prints tecnicofs tree.
 * Input:
 *  - fp: pointer to output file
 *  - current_time: initial time
 *  - final_time: time of ending
 */
void print_tecnicofs_tree(FILE *fp){
	/* print inode tree in file */
	inode_print_tree(fp, FS_ROOT, "");
}

/**
 * Blocks all "writing" threads and prints
 * the tecnicofs tree in the output file
 *
 * Input:
 * - fp: output file pointer
 */ 
void block_printfs(FILE *fp){
	lockInode(FS_ROOT, "W");
	print_tecnicofs_tree(fp);
	unlockInode(FS_ROOT);
}