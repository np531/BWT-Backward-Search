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
void printBits(size_t const size, void const * const ptr)
{
    unsigned char *b = (unsigned char*) ptr;
    unsigned char byte;
    int i, j;

    for (i = size-1; i >= 0; i--) {
        for (j = 7; j >= 0; j--) {
            byte = (b[i] >> j) & 1;
            printf("%u", byte);
        }
    }
    puts("");
}

/*
 *	Reads the file into memory
 */
void addToIndex(struct Index *index, char cur, int run) {
	// Append source array
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

	if (start == end) {
		return 1;
	}

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

/*
 *	Builds the occurrence and C tables used for indexing the BWT string
 */
void buildTables(struct Index *index) {
	// Construct the occ table
	int *prev = index->occ[0];
	for (int i = 0 ; i < strlen(index->source) ; i++) {
		// Update the Occ table
		if (i == 0) {
			index->occ[i][(int)index->source[i]]++;
		} else {
			index->occ = (int **)realloc(index->occ, (i+1)*sizeof(int *));
			if (index->occ == NULL) {
				printf("Unable to allocate memory for occ\n");
				exit(1);
			}

			index->occ[i] = (int *)calloc(ALPHABET_SIZE, sizeof(int));
			if (index->occ[i] == NULL) {
				printf("Unable to allocate memory for occ\n");
				exit(1);
			}
			memcpy(index->occ[i], prev, ALPHABET_SIZE*sizeof(int));
			index->occ[i][(int)index->source[i]]++;
			prev = index->occ[i];
		}
	}
	
	// Construct the C array
	int *finalOcc = (int *)malloc(sizeof(int)*127);
	int j = 0;
	memcpy(finalOcc, index->occ[index->count-1], sizeof(int)*127);
	for (int i = ALPHABET_SIZE - 1 ; i >= 0 ; i--) {
		if (finalOcc[i] != 0) {
			j = i - 1;
			while (j >= 0) {
				index->c[i] += finalOcc[j];
				j--;
			}
		}
	}
	free(finalOcc);
}

/*
 *	Reads the next char+binary run into a string to be processed
 *	Counts the number of bytes and adds it to posCount
 */
int getNextRun(struct Args *args, char *run, long *posCount) {
	char cur;
	run = (char *)realloc(run, sizeof(char));
	run[0] = '\0';

	// Read the first byte (a character) into memory
	if (!fread(&cur, 1, 1, args->rlbFile)) {
		return 0;
	}
	run = (char *)realloc(run, strlen(run) + 2);
	run = strncat(run, &cur, 1);
	(*posCount)++;

	// find all the subsequent run bytes
	if (!fread(&cur, 1, 1, args->rlbFile)) {
		return 1;
	}
	while ((cur >> 7) & 1) {
		(*posCount)++;
		run = (char *)realloc(run, strlen(run) + 2);
		run = strncat(run, &cur, 1);
		if (!fread(&cur, 1, 1, args->rlbFile)) {
			break;
		}
	} 
	fseek(args->rlbFile, -1, SEEK_CUR);

	return 1;
}

void decodeRLB(struct Args *args, struct Index *index) {
	long posCount = 0;
	/* long lCount = 0; */
	char *run = (char *)malloc(sizeof(char));
	/* int *curOcc = (int *)calloc(ALPHABET_SIZE, sizeof(int)); */
	run[0] = '\0';

	/* for (int i = 0 ; i < ALPHABET_SIZE ; i++) { */
	/* 	printf("%c : %d\n", (char)i, curOcc[i]); */
	/* } */

	// Continually read each count from rlb file
	while (getNextRun(args, run, &posCount)) { 
		/* for (int i = 0; i < strlen(run) ; i++) { */
		/* 	printf("%c", run[i]); */
		/* } */
		addToIndex(index, run[0], runToInt(run, 0, strlen(run)-1));
	}

	//TODO - pos in file is posCount-1
	//TODO - Integrate gapCount and occ table construction into the initial rlb decode
	//TODO - Store index to RLB and count through current run (for each occ)
	//TODO - Rebuild OCC array structure (using array of new Occ structs)

	/* printf("\n"); */
	/* printf("%ld\n", posCount); */
	/* printf("%s\n", index->source); */
	free(run);
}

int main(int argc, char **argv) {
	// Initialise data structures
	int indexExists = 0;
	struct Args *args = parseArgs(argc, argv, &indexExists);
	struct MatchList *matches = initMatchList();
	struct Index *index = initIndex();
	
	// Convert RLB to BWT 
	decodeRLB(args, index);
	buildTables(index);
	matches = findMatches(index, matches, args->pattern);
	printMatches(matches);

	// Free data structures
	freeIndex(index);
	freeMatchList(matches);
	freeArgs(args);
	return 0;
}

struct Occ *initOccBlock(long rlbPos, int *curOcc, int curRunCount) {
	struct Occ *newOcc = (struct Occ *)malloc(sizeof(struct Occ));
	if (newOcc == NULL) {
		printf("Unable to allocate OccBlock\n");
		exit(1);
	}

	// TODO - CHECK THAT THE -1 MAKES IT POINT TO THE CORRECT POS
	newOcc->rlbPos = rlbPos - 1;
	newOcc->occ = (int *)malloc(ALPHABET_SIZE*sizeof(int));
	newOcc->occ = memcpy(newOcc->occ, curOcc, ALPHABET_SIZE*sizeof(int));
	newOcc->curRunCount = curRunCount;

	return newOcc;
}

void freeIndex(struct Index *index) {
	if (index->source != NULL) {
		free(index->source);
	}
	// TODO - free occ array
	/* if (index->occ != NULL) { */
	/* 	int i = 0; */
	/* 	while (i < index->count) { */
	/* 		free(index->occ[i]); */
	/* 		i++; */
	/* 	} */
	/* 	/1* free(index->occ); *1/ */
	/* } */

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
	return newMatchList;
}

struct Args *parseArgs(int argc, char **argv, int *indexExists) {
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
	fseek(args->rlbFile, 0, SEEK_END);
	args->rlbSize = ftell(args->rlbFile);
	fseek(args->rlbFile, 0, SEEK_SET);
	
	// Check if index already exists, else create it
	args->indexFile = fopen(argv[2], "rb");
	if (args->indexFile) {
		*indexExists = 1;
	} else {
		args->indexFile = fopen(argv[2], "w+");
		*indexExists = 0;
	}

	return args;
}

void freeArgs(struct Args *args) {
	fclose(args->rlbFile);
	fclose(args->indexFile);
	free(args->pattern);
	free(args);
}


/* char *parseRLBString(struct Args *args) { */
/* 	fseek(args->rlbFile, 0, SEEK_END); */
/* 	long fileSize = ftell(args->rlbFile); */
/* 	char *source = (char *)malloc((fileSize+1)*sizeof(char)); */

/* 	fseek(args->rlbFile, 0, SEEK_SET); */
/* 	long sourceSize = fread(source, sizeof(char), fileSize, args->rlbFile); */
/* 	source[sourceSize] = '\0'; */
/* 	return source; */
/* } */
