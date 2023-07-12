#ifndef BWTSEARCH_H
#define BWTSEARCH_H

#include <stdio.h>
#include <inttypes.h>
#include "bwt.h"

#define PATTERN_MAX 512
#define MIN_RUN 3
extern char* strdup(const char*);

struct Args {
    FILE *rlbFile;
    FILE *indexFile;
    char *pattern;
};

struct Index {
	long count;
	char *S;
	uint32_t *B;
};

struct Args *parseArgs(int argc, char **argv);
void freeArgs(struct Args *args);

void freeMatchList(struct MatchList *matches); 
struct MatchList *initMatchList(void);

struct Index *initIndex(void);
void freeIndex(struct Index *index);

char *parseRLBString(struct Args *args);

#endif
