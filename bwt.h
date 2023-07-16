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


struct MatchList *searchBWT(struct MatchList *matches, char *source, char *pattern);
void freeMatchList(struct MatchList *matches);
struct MatchList *initMatchList(void);
void buildTables(struct Index *index);


#endif

