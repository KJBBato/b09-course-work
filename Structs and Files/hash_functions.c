#include <stdio.h>
#include <stdlib.h>

/* Computes Hash Value of the given FILE */
char *hash(FILE *fp) {
	// Allocates memory for an 8-byte hash
	char *hash_out = (char *)malloc(9*sizeof(char));
	// Initialize the hash value
	hash_out[0] = '\0';
	hash_out[1] = '\0';
	hash_out[2] = '\0';
	hash_out[3] = '\0';
	hash_out[4] = '\0';
	hash_out[5] = '\0';
	hash_out[6] = '\0';
	hash_out[7] = '\0';
	// Initialize values needed
    char c;
    int i = 0;
    // Loops through each character of the file
    while (fscanf(fp,"%c", &c) != EOF) {
    	// XOR individual character
    	hash_out[i] = c ^ hash_out[i];
    	// Increment the counter
    	i++;
    	// Set i to 0 if it reaches the end of the hash value
		if (i == 8) {
			i = 0;
			}
    }
    return hash_out;
}

