#ifndef BWTSEARCH_H
#define BWTSEARCH_H

#include <stdio.h>
#include <inttypes.h>

#define PATTERN_MAX 512
#define ALPHABET_SIZE 127
#define MIN_RUN 3
#define SMALL_FILE_MAX 1 
extern char* strdup(const char*);

struct Args {
    FILE *rlbFile;
    FILE *indexFile;
    char *pattern;
};

struct Occ {
	long rlbPos;
	int curRunCount;
	int occ[ALPHABET_SIZE];
};

struct Index {
	long count;
	char *source;
	int *c;
	/* struct Occ *occArray; */
	long rlbSize;
	int numGaps;
	int gapSize;
};

struct Args *parseArgs(int argc, char **argv, int *indexExists);
void freeArgs(struct Args *args);

struct Occ *initOccBlock(long rlbPos, int *curOcc, int curRunCount);
void freeOccBlock(struct Occ *occ);
void initCTable(struct Index *index, int *lastOcc);

struct Index *initIndex(struct Args *args);
void freeIndex(struct Index *index);

int runToInt(char *source, long start, long end);
int getNextRun(struct Args *args, char *run, long *start, long *end);
long getBwtSize(struct Args *args, struct Index *index);
char *parseRLBString(struct Args *args);
void decodeRLB(struct Args *args, struct Index *index);

#endif
