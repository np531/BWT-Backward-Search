#ifndef BWTSEARCH_H
#define BWTSEARCH_H

#include <stdio.h>
#include <inttypes.h>

#define PATTERN_MAX 512
#define ALPHABET_SIZE 127
#define MIN_RUN 3
#define OCC_GAP 4
extern char* strdup(const char*);

struct Args {
    FILE *rlbFile;
    FILE *indexFile;
    char *pattern;
	long rlbSize;
};

struct Occ {
	long rlbPos;
	int *occ;
	int curRunCount;
};

struct Index {
	long count;
	char *source;
	int *c;
	int **occ;
};

struct Args *parseArgs(int argc, char **argv, int *indexExists);
void freeArgs(struct Args *args);

struct Occ *initOccBlock();
void freeOccBlock(struct Occ *occ);

struct Index *initIndex(void);
void freeIndex(struct Index *index);

char *parseRLBString(struct Args *args);
void decodeRLB(struct Args *args, struct Index *index);

#endif
