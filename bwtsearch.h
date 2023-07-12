#ifndef BWTSEARCH_H
#define BWTSEARCH_H

#include <stdio.h>
#include "bwt.h"

#define PATTERN_MAX 512
extern char* strdup(const char*);

struct Args {
    FILE *rlbFile;
    FILE *indexFile;
    char *pattern;
};

struct Index {
	long count;
	char *S;
	char *B;
};

struct Args *parseArgs(int argc, char **argv);
void freeArgs(struct Args *args);

void freeMatchList(struct MatchList *matches); 
struct MatchList *initMatchList(void);

struct Index *initIndex(void);

char *parseRLBString(struct Args *args);

#endif
