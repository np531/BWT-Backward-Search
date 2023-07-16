#ifndef BWTSEARCH_H
#define BWTSEARCH_H

#include <stdio.h>
#include <inttypes.h>

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
	char *source;
	int *c;
};

struct Args *parseArgs(int argc, char **argv);
void freeArgs(struct Args *args);

struct Index *initIndex(void);
void freeIndex(struct Index *index);

char *parseRLBString(struct Args *args);

#endif
