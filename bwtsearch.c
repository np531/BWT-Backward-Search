#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <inttypes.h>
#include <stdbool.h>
#include <math.h>
#include "bwtsearch.h"
#include "bwt.h"

// Assumes little endian
/* void printBits(size_t const size, void const * const ptr) */
/* { */
/*     unsigned char *b = (unsigned char*) ptr; */
/*     unsigned char byte; */
/*     int i, j; */

/*     for (i = size-1; i >= 0; i--) { */
/*         for (j = 7; j >= 0; j--) { */
/*             byte = (b[i] >> j) & 1; */
/*             printf("%u", byte); */
/*         } */
/*     } */
/*     puts(""); */
/* } */

/* void addToB(struct Index *index, int run) { */
/* 	// Append B Array */
/* 	int curSize = ceil(index->count/32.0); */
/* 	int newSize = ceil((index->count+run)/32.0); */
/* 	int offset = index->count % 32; */

/* 	// Store bits from LSB first */
/* 	if (newSize > curSize) { */
/* 		// A new int is needed in the bit array */
/* 		printf("curSize: %d, newSize: %d\n", curSize, newSize); */
/* 		index->B = (uint32_t *)realloc(index->B, newSize*sizeof(uint32_t)); */
/* 		if (index->count == 0) { */
/* 			index->B[curSize] |= 1 << offset; */

/* 		} else{ */
/* 			index->B[curSize - 1] |= 1 << offset; */
/* 		} */
/* 	} else { */
/* 		index->B[curSize - 1] |= 1 << offset; */
/* 	} */
/* } */

/*
 *	Adds a given character and run length to the RLB index (S,B,B' arrays)
 */
void addToIndex(struct Index *index, char cur, int run) {
	// Append S array
	int temp = run;
	index->source = (char *)realloc(index->source, index->count + run*sizeof(char) + 1);
	while (temp > 0) {
		index->source = strncat(index->source, &cur, 1);
		temp--;
	}

	if (run == 1) {
		index->count++;
	} else if (run >= MIN_RUN) {
		index->count += run;
	} else {
		printf("INVALID RUN LENGTH\n");
		exit(1);
	}
}

/*
 * Converts consecutive bytes representing a run into the decimal value
 * of that long (adding the +3)
 */
int runToInt(char *source, long start, long end) {
	uint64_t run = 0;
	unsigned char cur = 0;
	unsigned char bitmask = 0x7F;

	// Reconstruct the bytes back into the size of the run
	while (end > start) {
		cur = source[end] & bitmask;
		run |= cur;
		run <<= 7;
		end--;
	}
	run >>= 7;

	return run + MIN_RUN;	
}

void decodeRLB(char *source, struct Index *index) {
	unsigned char cur;
	unsigned char temp;
	long start;
	long end;
	int run;
	// Iterate through each byte
	for (long i = 0; i < strlen(source); i++) {
		cur = source[i];

		if ((cur>>7)&1) {
			printf("ENCOUNTERED RUN BEFORE CHAR EXITING\n");
			exit(1);	
		}

		start = i; 
		end = i+1;
		temp = source[end];
		if (!((temp >> 7) & 1)) {
			// Add straight to index if the following byte isnt a run
			addToIndex(index, (char)cur, 1);
		} else {
			// Find the start and end indexes of the run count bytes
			while ((temp >> 7) & 1) {
				end++;
				temp = source[end];
			}
			end--;
			i = end;

			run = runToInt(source, start, end);
			addToIndex(index, (char)cur, run);
		}
	}

	// Add terminating 1 to B array
	/* addToB(index, 0); */	

	/* if (index->source != NULL) { */
	/* 	printf("%s\n", index->source); */
	/* 	printf("%ld\n", index->count); */
	/* 	printf("%ld\n",strlen(index->source)); */
	/* } */	
}

int main(int argc, char **argv) {
	// Initialise data structures
	struct Args *args = parseArgs(argc, argv);
	char *source = parseRLBString(args);
	struct MatchList *matches = initMatchList();
	struct Index *index = initIndex();

	// Execute search
	/* printf("%d\n", matches->size); */
	decodeRLB(source, index);
	buildTables(index);
	matches = findMatches(index, matches, args->pattern);
	printMatches(matches);

	// Free data structures
	freeIndex(index);
	freeMatchList(matches);
	freeArgs(args);
	return 0;
}

void freeIndex(struct Index *index) {
	if (index->source != NULL) {
		free(index->source);
	}
	if (index->occ != NULL) {
		int i = 0;
		while (i < index->count) {
			free(index->occ[i]);
			i++;
		}
		/* free(index->occ); */
	}

	free(index->c);
	free(index);
}

struct Index *initIndex(void) {
	struct Index *index = (struct Index *)malloc(sizeof(struct Index));
	if (index == NULL) {
		printf("Unable to allocate memory for index\n");
		exit(1);
	}
	index->count = 0;
	index->source = NULL;

	index->c = (int *)malloc(sizeof(int)*ALPHABET_SIZE);
	// Initialise the array to empty
	for (int i = 0 ; i < ALPHABET_SIZE ; i++) {
		index->c[i] = 0;
	}

	index->occ = (int **)malloc(sizeof(int *));
	if (index->occ == NULL) {
		printf("Unable to allocate initial memory for occ\n");
		exit(1);
	}
	index->occ[0] = (int *)calloc(ALPHABET_SIZE, sizeof(int));
	if (index->occ[0] == NULL) {
		printf("Unable to allocate initial memory for occ\n");
		exit(1);
	}
	return index;
}

void freeMatchList(struct MatchList *matches) {
	struct Match *cur = matches->head;
	struct Match *next;

	while (cur != NULL) {
		next = cur->next;
		free(cur);
		cur = next;
	}

	free(matches);
}

struct MatchList *initMatchList() {
	struct MatchList *newMatchList = (struct MatchList *)malloc(sizeof(struct MatchList));
	newMatchList->head = NULL;
	/* newMatchList->matches = NULL; */
	/* newMatchList->size = 0; */
	return newMatchList;
}

struct Args *parseArgs(int argc, char **argv) {
	if (argc != 4) {
		printf("Incorrect number of arguments provided\n");
		exit(1);
	}

	struct Args *args = (struct Args *)malloc(sizeof(struct Args));
	if (args == NULL) {
		printf("Memory allocation error when initialising arg container\n");
		exit(1);
	}

	args->pattern = strdup(argv[3]);
	args->rlbFile = fopen(argv[1], "rb");
	if (args->rlbFile == NULL) {
		printf("rlb file does not exist\n");
		exit(1);
	}
	args->indexFile = fopen(argv[2], "w+");

	return args;
}

void freeArgs(struct Args *args) {
	fclose(args->rlbFile);
	fclose(args->indexFile);
	free(args->pattern);
	free(args);
}


char *parseRLBString(struct Args *args) {
	fseek(args->rlbFile, 0, SEEK_END);
	long fileSize = ftell(args->rlbFile);
	char *source = (char *)malloc((fileSize+1)*sizeof(char));

	fseek(args->rlbFile, 0, SEEK_SET);
	long sourceSize = fread(source, sizeof(char), fileSize, args->rlbFile);
	source[sourceSize] = '\0';
	return source;
}
