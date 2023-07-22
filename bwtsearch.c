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
/* void buildTables(struct Index *index) { */
/* 	// Construct the occ table */
/* 	int *prev = index->occ[0]; */
/* 	for (int i = 0 ; i < strlen(index->source) ; i++) { */
/* 		// Update the Occ table */
/* 		if (i == 0) { */
/* 			index->occ[i][(int)index->source[i]]++; */
/* 		} else { */
/* 			index->occ = (int **)realloc(index->occ, (i+1)*sizeof(int *)); */
/* 			if (index->occ == NULL) { */
/* 				printf("Unable to allocate memory for occ\n"); */
/* 				exit(1); */
/* 			} */

/* 			index->occ[i] = (int *)calloc(ALPHABET_SIZE, sizeof(int)); */
/* 			if (index->occ[i] == NULL) { */
/* 				printf("Unable to allocate memory for occ\n"); */
/* 				exit(1); */
/* 			} */
/* 			memcpy(index->occ[i], prev, ALPHABET_SIZE*sizeof(int)); */
/* 			index->occ[i][(int)index->source[i]]++; */
/* 			prev = index->occ[i]; */
/* 		} */
/* 	} */
	
/* 	// Construct the C array */
/* 	int *finalOcc = (int *)malloc(sizeof(int)*127); */
/* 	memcpy(finalOcc, index->occ[index->count-1], sizeof(int)*127); */
/* 	initCTable(index, finalOcc); */
/* 	free(finalOcc); */
/* } */

/*
 *	Reads the next char+binary run into a string to be processed
 *	Counts the number of bytes and adds it to posCount
 */
int getNextRun(struct Args *args, char *run, long *start, long *end) {
	char cur;
	run = (char *)realloc(run, sizeof(char));
	run[0] = '\0';

	// Read the first byte (a character) into memory
	if (!fread(&cur, 1, 1, args->rlbFile)) {
		return 0;
	}
	run = (char *)realloc(run, strlen(run) + 2);
	run = strncat(run, &cur, 1);
	(*start)++;
	(*end)++;

	// find all the subsequent run bytes
	if (!fread(&cur, 1, 1, args->rlbFile)) {
		return 1;
	}
	while ((cur >> 7) & 1) {
		(*end)++;
		run = (char *)realloc(run, strlen(run) + 2);
		run = strncat(run, &cur, 1);
		if (!fread(&cur, 1, 1, args->rlbFile)) {
			break;
		}
	} 
	fseek(args->rlbFile, -1, SEEK_CUR);

	return 1;
}

/*
 * decodes entire RLB string into memory
 * Constructs the C table (occ table generated from memory for smaller files)
 */
void decodeRLBSlow(struct Args *args, struct Index *index) {
	long start = 0;
	long end = 0;
	long lCount = -1;

	char *run = (char *)malloc(sizeof(char));
	int *curOcc = (int *)calloc(ALPHABET_SIZE, sizeof(int));
	curOcc[0] = 0;
	run[0] = '\0';

	int temp = 0;
	int runLen = 0;

	// Continually read each count from rlb file
	while (getNextRun(args, run, &start, &end)) { 
		runLen = runToInt(run, 0, strlen(run)-1);
		temp = runLen;

		index->source = (char *)realloc(index->source, lCount + 1 + temp*sizeof(char) + 1);
		while (temp > 0) {
			index->source = strncat(index->source, run , 1);
			curOcc[(int)run[0]]++;
			temp--;
			lCount++;
		}
	}

	// Construct the C array
	initCTable(index, curOcc);

	index->count = lCount+1;
	free(curOcc);
	free(run);
}

long getBwtSize(struct Args *args, struct Index *index) {
	long start = 0;
	long  end = 0;
	char *run = (char *)malloc(sizeof(char));
	run[0] = '\0';
	long lCount = -1;

	int temp = 0;
	int runLen = 0;
	while (getNextRun(args, run, &start, &end)) { 
		runLen = runToInt(run, 0, strlen(run)-1);
		temp = runLen;
		index->source = (char *)realloc(index->source, lCount + 1 + temp*sizeof(char) + 1);

		while (temp > 0) {
			index->source = strncat(index->source, run , 1);
			temp--;
			lCount++;
		}
	}
	index->count = lCount+1;
	fseek(args->rlbFile, 0, SEEK_SET);
	return lCount+1;

}

void decodeRLB(struct Args *args, struct Index *index) {
	long start = 0;
	long end = 0;
	long lCount = -1;

	struct Occ *newOcc = NULL; 
	char *run = (char *)malloc(sizeof(char));
	int *curOcc = (int *)calloc(ALPHABET_SIZE, sizeof(int));
	run[0] = '\0';

	int temp = 0;
	int runLen = 0;

	// Continually read each character run from rlb file
	while (getNextRun(args, run, &start, &end)) { 
		runLen = runToInt(run, 0, strlen(run)-1);
		temp = runLen;

		while (temp > 0) {
			curOcc[(int)run[0]]++;
			temp--;
			lCount++;
			
			if (lCount % index->gapSize == 0) {
				newOcc = initOccBlock(start - 1, curOcc, runLen-temp);
				fwrite(newOcc, sizeof(struct Occ), 1, args->indexFile);
				free(newOcc);
			}
		}
		start=end;
	}

	// Construct the C array
	initCTable(index, curOcc);
	fwrite(index->c, sizeof(index->c), 1, args->indexFile);

	index->count = lCount+1;
	free(curOcc);
	free(run);
}

int main(int argc, char **argv) {
	// Initialise data structures
	int indexExists = 0;
	struct Args *args = parseArgs(argc, argv, &indexExists);
	struct MatchList *matches = initMatchList();
	struct Index *index = initIndex(args);
	
	
	// Convert RLB to BWT 
	if (index->rlbSize <= SMALL_FILE_MAX) {
		decodeRLBSlow(args, index);
		matches = findMatches(args, index, matches, args->pattern);
		printMatches(matches);

	} else {
		/* printf("large file\n"); */
		// Calculate the gap size to fit Occ objects and C table
		long bwtSize = getBwtSize(args, index);
		index->gapSize = (int)(bwtSize)/index->numGaps;
		while(index->gapSize*index->numGaps < bwtSize + (sizeof(int)*ALPHABET_SIZE)) {
			index->gapSize += 5;
		}

		//TODO - REMOVE ME
		/* index->gapSize = 1; */

		if (indexExists) {
			int *curC = (int *)malloc(sizeof(index->c));
			/* printf("Index exists\n"); */
			
			// Read C table
			fseek(args->indexFile, 0, SEEK_END);
			long indexSize = ftell(args->indexFile);
			fseek(args->indexFile, indexSize-sizeof(index->c), SEEK_SET);
			if (fread(curC, sizeof(index->c), 1, args->indexFile) == 0) {
				printf("FAILED TO READ C TABLE FROM INDEX\n");
				exit(1);
			}

			memcpy(index->c, curC, sizeof(index->c));
			/* printf("%ld\n", indexSize-sizeof(index->c)); */

			free(curC);

		} else {
			decodeRLB(args, index);
		}
		/* for (int i = 0 ; i < ALPHABET_SIZE ; i++) { */
		/* 	printf("%c | %d\n", (char)i, index->c[i]); */
		/* } */

		/* printf("%s\n", index->source); */
		/* int result = rank(args, index, 'p', 8); */
		/* printf("|%d|\n", result); */
		/* char result = getBwtChar(args, index, 1); */
		/* printf("|%c|\n", result); */

		matches = findMatches(args, index, matches, args->pattern);
		printMatches(matches);
	}

	// Free data structures
	freeIndex(index);
	freeMatchList(matches);
	freeArgs(args);
	return 0;
}

void initCTable(struct Index *index, int *lastOcc) {
	int j = 0;
	for (int i = ALPHABET_SIZE - 1 ; i >= 0 ; i--) {
		if (lastOcc[i] != 0) {
			j = i - 1;
			while (j >= 0) {
				index->c[i] += lastOcc[j];
				j--;
			}
		}
	}

}

struct Occ *initOccBlock(long rlbPos, int *curOcc, int curRunCount) {
	struct Occ *newOcc = (struct Occ *)malloc(sizeof(struct Occ));
	if (newOcc == NULL) {
		printf("Unable to allocate OccBlock\n");
		exit(1);
	}

	newOcc->rlbPos = rlbPos;
	memcpy(newOcc->occ, curOcc, ALPHABET_SIZE*sizeof(int));
	newOcc->curRunCount = curRunCount;

	return newOcc;
}

void freeIndex(struct Index *index) {
	if (index->source != NULL) {
		free(index->source);
	}

	// TODO - IF C TABLE CHANGE BREAKS THINGS, RETURN TO POINTER
	/* free(index->c); */
	free(index);
}

struct Index *initIndex(struct Args* args) {
	struct Index *index = (struct Index *)malloc(sizeof(struct Index));
	if (index == NULL) {
		printf("Unable to allocate memory for index\n");
		exit(1);
	}
	index->count = 0;
	index->source = NULL;

	/* index->c = (int *)malloc(sizeof(int)*ALPHABET_SIZE); */
	// Initialise the c array to empty
	for (int i = 0 ; i < ALPHABET_SIZE ; i++) {
		index->c[i] = 0;
	}

	fseek(args->rlbFile, 0, SEEK_END);
	index->rlbSize = ftell(args->rlbFile);
	fseek(args->rlbFile, 0, SEEK_SET);

	/* index->occ = (int **)malloc(sizeof(int *)); */
	/* if (index->occ == NULL) { */
	/* 	printf("Unable to allocate initial memory for occ\n"); */
	/* 	exit(1); */
	/* } */
	/* index->occ[0] = (int *)calloc(ALPHABET_SIZE, sizeof(int)); */
	/* if (index->occ[0] == NULL) { */
	/* 	printf("Unable to allocate initial memory for occ\n"); */
	/* 	exit(1); */
	/* } */

	/* index->occArray = NULL; */
	// TODO - FIX UP SMALL (<1000 size rlb files once gaps are working)
	if (index->rlbSize >= sizeof(struct Occ)+10) {
		int numGaps = (int)(ceil(index->rlbSize)/(sizeof(struct Occ)));
		index->numGaps = numGaps;
		/* printf("numGaps: %d\n", numGaps); */
		/* printf("gapSize: %d\n", index->gapSize); */
	} else {
		index->numGaps = 1;
		index->gapSize = 1;
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
	
	// Check if index already exists, else create it
	args->indexFile = fopen(argv[2], "rb");
	if (args->indexFile) {
		*indexExists = 1;
	} else {
		args->indexFile = fopen(argv[2], "w+b");
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

