#ifndef BWTSEARCH_H
#define BWTSEARCH_H

#include <stdio.h>

#define PATTERN_MAX 512
extern char* strdup(const char*);

struct Args {
    FILE *rlbFile;
    FILE *indexFile;
    char *pattern;
};

struct Args *parseArgs(int argc, char **argv);
struct MatchList *initMatchList(void);
void freeArgs(struct Args *args);
char *parseBWTString(struct Args *args);

#endif

