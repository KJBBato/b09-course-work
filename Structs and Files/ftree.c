#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>

#include "ftree.h"
#include "hash.h"

/* Function Prototypes */
struct TreeNode *build_fnode(const char *path, struct stat buf);
char *create_relative_path(const char *curr_path, char *curr_fname);
void print_ftree_helper(struct TreeNode *root, int depth);

/*
 * Returns the FTree rooted at the path fname.
 */
struct TreeNode *generate_ftree(const char *fname) {

	// Obtain information of a file
	struct stat buf;
	lstat(fname, &buf);

	// Declaration of TreeNode
	struct TreeNode *root;

	//Build File Node
	root = build_fnode(fname, buf);

	// Checks if fname is a directory
	if (S_ISDIR(buf.st_mode)) {

		// Declare directory pointer and struct
		DIR *dir;
		struct dirent *temp_dir;
		// Open the directory of a given file path
		dir = opendir(fname);
		// If the directory cannot be opened
		if (dir == NULL) {
			fprintf(stderr, "Error: Unable to open '%s' directory\n", root->fname);
			exit(1);
		} else {
			// Initialize nodes needed to create a linked list
			struct TreeNode *prevNode = NULL;
			struct TreeNode *currNode = NULL;
			// Goes through the contents of a directory
			while((temp_dir = readdir(dir)) != NULL) {
				// Creates a relative path for the new file
				char *path;
				char c = temp_dir->d_name[0];
				path = create_relative_path(fname, temp_dir->d_name);
				// Checks if the file starts with '.'
				if (c != '.') {
					// Generate the File node using the new path given
					currNode = generate_ftree(path);
					// Checks if it is a directory with no contents (field)
					if (root->contents == NULL &&
							root->hash == NULL &&
							temp_dir->d_type != DT_UNKNOWN) {

						// Initialize contents
						root->contents = currNode;
						prevNode = currNode;

					}
					// Otherwise, link nodes together
					else
					{
						currNode->next = NULL;
						prevNode->next = currNode;
						prevNode = currNode;

					}
				}
				// Free up path, as we will be using it often
				free(path);
			}
		}
		// Close the current directory
		closedir(dir);
	}

	// Checks if the File is a regular file or a link
	else if (S_ISREG(buf.st_mode) | S_ISLNK(buf.st_mode)) {
		// Declare file pointer
		FILE *fp;
		// Get File stream
		fp = fopen(fname, "r");
		// If file cannot open
		if (fp == NULL) {
			fprintf(stderr, "Error: There is no such '%s' file that exists\n", root->fname);
			exit(1);
		}
		// Proceed by taking the hash of a regular file or link
		else {
			root->hash = (char *)malloc(9);
			strcpy(root->hash, hash(fp));
		}
		// Close the current file
		fclose(fp);
	}
	// Neither file nor directory
	else {

		fprintf(stderr, "lstat: the name '%s' is not a file or directory\n", root->fname);
		exit(1);
	}
	return root;
}


/*
 * Returns an initialized TreeNode with its filename and permission
 */
struct TreeNode *build_fnode(const char *path, struct stat buf) {

	// Declare new TreeNode
	struct TreeNode *node = (struct TreeNode *)malloc(sizeof(struct TreeNode));
	// Set fields to NULL and will later be set
	node->contents = NULL;
	node->hash = NULL;
	node->next = NULL;
	// Checks if path is a relative path or a file name to set fname field
	char *slash_ptr;
	slash_ptr = strrchr(path, '/');
	if (slash_ptr != NULL) {
		node->fname = (char *)malloc((strlen(slash_ptr)+1)*sizeof(char));
		strcpy(node->fname, &slash_ptr[1]);
	} else {
		node->fname = (char *)malloc((strlen(path)+1)*sizeof(char));
		strcpy(node->fname, path);
	}
	// Sets permission in octal value
	int perm;
	perm = buf.st_mode & 0777;
	node->permissions = perm;
	return node;
}

/*
 * Creates a relative path for a given current directory and filename
 */
char *create_relative_path(const char *curr_path, char *curr_fname) {
	int str_size;
	char *res_path;
	str_size = strlen(curr_path) + strlen(curr_fname) + 1;
	res_path = (char *)malloc(strlen(curr_path)+1);
	strcpy(res_path, curr_path);
	res_path = (char *)realloc(res_path, strlen(curr_path) + 2);
	strcat(res_path, "/");
	res_path = (char *)realloc(res_path, str_size + 1);
	strcat(res_path, curr_fname);
	return res_path;
}

/*
 * Prints the TreeNodes encountered on a preorder traversal of an FTree.
 */
void print_ftree(struct TreeNode *root) {
    int depth = 0;
	printf("%*s", depth * 2, "");
    print_ftree_helper(root, depth);
}

/*
 * FTree print helper function that recursively prints the root node
 */
void print_ftree_helper(struct TreeNode *root, int depth) {

	printf("%*s", depth * 2, "");
	if (root->hash == NULL) {
		printf("===== %s (%o) =====\n", root->fname, root->permissions);
	}
	else {
		printf("%s (%o)\n", root->fname, root->permissions);
	}
	if (root->contents != NULL) {
		print_ftree_helper(root->contents, depth+1);
	}
	if (root->next != NULL) {
		print_ftree_helper(root->next, depth);
	}
}