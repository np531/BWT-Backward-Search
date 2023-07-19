#ifndef BWT_H
#define BWT_H

#include "bwtsearch.h"

extern char* strdup(const char*);

struct Match {
	int record;
	char *string;
};

struct MatchList {
	struct Match *matches;
	int size;
};


int getNextC(struct Index *index, char curChar);
struct MatchList *searchBWT(struct Index *index, struct MatchList *matches, char *pattern);
void freeMatchList(struct MatchList *matches);
struct MatchList *initMatchList(void);
struct Match *initMatch(int record, char *string);
void buildTables(struct Index *index);


#endif

