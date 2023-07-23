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

/*
 * Decodes the rlb file to find the size of the original BWT text
 * Used for calculating gapSize (necessary for rlbDecode)
 */
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
		while (temp > 0) {
			temp--;
			lCount++;
		}
	}
	index->count = lCount+1;
	fseek(args->rlbFile, 0, SEEK_SET);
	return lCount+1;

}

// TODO
/*
 *	Increase the buffer size being read in from the rlb file during index creation
 *  - Extend getNextRun so it reads a 512KB? buffer into memory (checking to ensure
 *    the end matches up with the end of a run)
 *  - Extend decodeRlb and getBwtSize to handle this change, updating index accordingly
 *  - Also find why 'Ship' is so goddamn slow on small2 lmao
 */ 

int getNextChunk(struct Index *index, struct Args *args, char *chunk) {
	char cur;

	// Read the chunk into memory
	if (!fread(chunk, CHUNK_SIZE, 1, args->rlbFile)) {
		return index->rlbSize%CHUNK_SIZE;
	}
	chunk[CHUNK_SIZE] = '\0';


	// find all the subsequent run bytes
	if (!fread(&cur, 1, 1, args->rlbFile)) {
		return -1;
	}
	while ((cur >> 7) & 1) {
		/* chunk = (char *)realloc(chunk, strlen(chunk) + 2); */
		chunk = strncat(chunk, &cur, 1);
		if (!fread(&cur, 1, 1, args->rlbFile)) {
			break;
		}
	} 
	fseek(args->rlbFile, -1, SEEK_CUR);

	return -1;
}

int getNextChunkRun(struct Args *args, char* chunk, char *run, long *chunkPointer, long *start, long *end) {
	char cur;
	long chunkLen = strlen(chunk);
	run = (char *)realloc(run, sizeof(char));
	run[0] = '\0';

	// Read the first byte (a character) into memory
	cur = chunk[*chunkPointer];
	(*chunkPointer)++;
	if (*chunkPointer > chunkLen) {
		/* printf("%c\n", cur); */
		/* printf("b\n"); */
		/* run = (char *)realloc(run, strlen(run) + 2); */
		/* run = strncat(run, &cur, 1); */
		return 0;
	}
	run = (char *)realloc(run, strlen(run) + 2);
	run = strncat(run, &cur, 1);
	(*start)++;
	(*end)++;

	// find all the subsequent run bytes
	cur = chunk[*chunkPointer];
	(*chunkPointer)++;
	if (*chunkPointer > chunkLen) {
		return 1;
	}
	while ((cur >> 7) & 1) {
		(*end)++;
		run = (char *)realloc(run, strlen(run) + 2);
		run = strncat(run, &cur, 1);

		cur = chunk[*chunkPointer];
		(*chunkPointer)++;
		if (*chunkPointer > chunkLen) {
			return 1;
		}
	} 
	(*chunkPointer)--;

	return 1;
}

void decodeRLBFast(struct Args *args, struct Index *index) {
	long start = 0;
	long end = 0;
	long lCount = -1;

	struct Occ *newOcc = NULL; 
	char *chunk = (char *)malloc(CHUNK_SIZE*sizeof(char)+50);
	char *run= (char *)malloc(sizeof(char));
	int *curOcc = (int *)calloc(ALPHABET_SIZE, sizeof(int));

	int temp = 0;
	int runLen = 0;

	long chunkPointer = 0;

	// Continually read each character run from rlb file
	int size = getNextChunk(index, args, chunk);
	while (size == -1) { 
		/* printf("%s - len:%ld\n", chunk, strlen(chunk)); */
		/* printf("a - chunk: '%s'\n", chunk); */
		while (getNextChunkRun(args, chunk, run, &chunkPointer, &start, &end)) {
			/* printf("run - %s\n", run); */
			/* printBits(sizeof(char), &run[1]); */
			runLen = runToInt(run, 0, strlen(run)-1);
			/* printf("%c - %d\n", run[0], runLen); */

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
		chunkPointer = 0;
		size = getNextChunk(index, args, chunk);
	}
	/* printf("a - chunk: '%s' %d\n", chunk, size); */
	// Handle last chunk
	if (size > 0) {
		chunkPointer = 0;
		chunk[size] = '\0';
		/* printf("%s\n", chunk); */
		while (getNextChunkRun(args, chunk, run, &chunkPointer, &start, &end)) {
			/* printf("run - %s\n", run); */
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
	}

	// Construct the C array
	initCTable(index, curOcc);
	fwrite(index->c, sizeof(index->c), 1, args->indexFile);

	for (int i = 0 ; i < ALPHABET_SIZE ; i++) {
		printf("%c - %d\n", (char)i, index->c[i]);
	}
	
	// Store the size of the decoded BWT
	long bwtSize = lCount + 1;
	fwrite(&bwtSize, sizeof(long), 1, args->indexFile);

	index->count = bwtSize;
	free(curOcc);
	free(chunk);
	free(run);
}

/*
 * Decodes the rlb file, generating and writing to the index file
 * as it decodes the bwt text
 */
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

	// Store the size of the decoded BWT
	long bwtSize = lCount + 1;
	fwrite(&bwtSize, sizeof(long), 1, args->indexFile);

	index->count = bwtSize;
	free(curOcc);
	free(run);
}

/*
 *	Calculates a minimal gapSize that is less than the size of the rlb file provided
 */
void calcGapSize(struct Index *index, long bwtSize) {
	index->gapSize = (int)(bwtSize)/index->numGaps;
	while(index->gapSize*index->numGaps < bwtSize + (sizeof(int)*ALPHABET_SIZE) + sizeof(long)) {
		index->gapSize += 10;
	}
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
		if (indexExists) {
			int *curC = (int *)malloc(sizeof(index->c));
			
			// Read C table from index file
			fseek(args->indexFile, 0, SEEK_END);
			long indexSize = ftell(args->indexFile);
			fseek(args->indexFile, indexSize-sizeof(index->c)-sizeof(long), SEEK_SET);
			if (fread(curC, sizeof(index->c), 1, args->indexFile) == 0) {
				printf("FAILED TO READ C TABLE FROM INDEX\n");
				exit(1);
			}

			// Calculate the gapSize used in the index file
			long bwtSize;
			fseek(args->indexFile, indexSize-sizeof(long), SEEK_SET);
			if (fread(&bwtSize, sizeof(long), 1, args->indexFile) == 0) {
				printf("FAILED TO READ bwtSize FROM INDEX\n");
				exit(1);
			}
			index->count = bwtSize;
			calcGapSize(index, index->count);

			memcpy(index->c, curC, sizeof(index->c));
			free(curC);
		} else {
			// Generate index file
			long bwtSize = getBwtSize(args, index);
			calcGapSize(index, bwtSize);
			/* decodeRLB(args, index); */
			decodeRLBFast(args, index);
		}

		/* printf("a\n"); */
		matches = findMatches(args, index, matches, args->pattern);
		printMatches(matches);
	}

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

	/* index->occArray = NULL; */
	// TODO - FIX UP SMALL (<1000 size rlb files once gaps are working)
	if (index->rlbSize >= sizeof(struct Occ)+10) {
		int numGaps = (int)(ceil(index->rlbSize)/(sizeof(struct Occ))) - 1;
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

