#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <inttypes.h>
#include <stdbool.h>
#include "bwtsearch.h"
#include "bwt.h"

void charAsBinary(unsigned char c) {
    // Start from the most significant bit (MSB)
    unsigned char mask = 1 << (sizeof(unsigned char) * 8 - 1);

    // Iterate over each bit
    for (int i = 0; i < sizeof(unsigned char) * 8; i++) {
        // Check if the current bit is set
        if (c & mask) {
            printf("1");
        } else {
            printf("0");
        }

        // Shift the mask to the right to check the next bit
        mask >>= 1;
    }

    printf("\n");
}

void longAsBinary(unsigned long num) {
    // Start from the most significant bit (MSB)
    long mask = 1L << (sizeof(long) * 8 - 1);

    // Iterate over each bit
    for (int i = 0; i < sizeof(long) * 8; i++) {
        // Check if the current bit is set
        if (num & mask) {
            printf("1");
        } else {
            printf("0");
        }

        // Shift the mask to the right to check the next bit
        mask >>= 1;
    }

    printf("\n");
}

void intAsBinary(unsigned int num) {
    // Start from the most significant bit (MSB)
    int mask = 1 << (sizeof(int) * 8 - 1);

    // Iterate over each bit
    for (int i = 0; i < sizeof(int) * 8; i++) {
        // Check if the current bit is set
        if (num & mask) {
            printf("1");
        } else {
            printf("0");
        }

        // Shift the mask to the right to check the next bit
        mask >>= 1;
    }

    printf("\n");
}

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

void addToIndex(struct Index *index, char cur, int run) {
	if (run == 0) {
		index->S = (char *)realloc(index->S, index->count + sizeof(char) + 1);
		index->S = strncat(index->S, &cur, 1);
		index->count++;
	} else if (run >= 3) {

	} else {
		printf("INVALID RUN LENGTH");
		exit(1);
	}

}

/*
 * Converts consecutive bytes representing a run into the decimal value
 * of that long (adding the +3)
 */
uint64_t runToLong(char *source, long start, long end) {
	uint64_t run = 0;
	unsigned char cur = 0;
	unsigned char bitmask = 0x7F;

	while (end > start) {
		cur = source[end] & bitmask;
		printBits(sizeof(cur), &cur);
		run = run | cur;
		run <<= 7;

		end--;
	}
	run >>= 7;

	return run + 3;	
}

void decodeRLB(char *source, struct Index *index) {
	unsigned char cur;
	unsigned char temp;
	long start;
	long end;
	long run;
	for (long i = 0; i < strlen(source); i++) {
		cur = source[i];
		if (!(cur >> 7 & 1)) {
			start = i; 
			end = i+1;
			temp = source[end];
			if (!((temp >> 7) & 1)) {
				addToIndex(index, (char)cur, 0);
			} else {
				// Find the start and end indexes of the run count bytes
				while ((temp >> 7) & 1) {
					end++;
					temp = source[end];
				}
				end--;
				i = end;
				printf("Start->%ld, End->%ld\n", start, end);
				run = runToLong(source, start, end);
				printf("%ld\n", run);
			}
		} else {
			printf("FUCK\n");
		}
		/* charAsBinary(cur); */
	}
	if (index->S != NULL) {
		printf("%s\n", index->S);
	}	
	printf("\n");
}

int main(int argc, char **argv) {
	struct Args *args = parseArgs(argc, argv);
	printf("Pattern - %s\n", args->pattern);
	char *source = parseRLBString(args);

	struct MatchList *matches = initMatchList();
	/* matches = searchBWT(matches, source, args->pattern); */
	/* printf("%d\n", matches->size); */
	struct Index *index = initIndex();
	decodeRLB(source, index);
	// TODO - Remember to add \0 to S and find some way to terminate B (count?)
	// 		  Note that we might need to made multiple reads to the rlb
	// 		  file and populate the index in chunks, so the terminators
	// 		  should be added here, after all decodeRLBs are called.

	freeMatchList(matches);
	freeArgs(args);
	return 0;
}

struct Index *initIndex(void) {
	struct Index *index = (struct Index *)malloc(sizeof(struct Index));
	if (index == NULL) {
		printf("Unable to allocate memory for index\n");
		exit(1);
	}
	index->count = 0;
	index->S = NULL;
	index->B = NULL;
	return index;
}

void freeMatchList(struct MatchList *matches) {
	if (matches->matches != NULL) {
		free(matches->matches);
	}
	free(matches);
}

struct MatchList *initMatchList() {
	struct MatchList *newMatchList = (struct MatchList *)malloc(sizeof(struct MatchList));
	newMatchList->matches = NULL;
	newMatchList->size = 0;
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
