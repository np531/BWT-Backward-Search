#ifndef BWT_H
#define BWT_H

#include "bwtsearch.h"

extern char* strdup(const char*);
extern char* strndup(const char*, size_t n);

struct Match {
	struct Match *next;
	int record;
	char *string;
};

struct MatchList {
	struct Match *head;
};


int getNextC(struct Index *index, char curChar);
int rank(struct Index *index, char curChar, int line);

void reverseStr(char *str);
long getNextRecord(struct Index *index, int offset);
int searchBWT(struct Index *index, char *pattern, int *first, int *last);
char *extractStr(struct Index *index, int offset);

struct MatchList *findMatches(struct Index *index, struct MatchList *matches, char *pattern);
void addMatch(struct MatchList *matchList, struct Match *match);
void printMatches(struct MatchList *matchList);
struct MatchList *initMatchList(void);
struct Match *initMatch(int record, char *string);
void freeMatchList(struct MatchList *matches);

void buildTables(struct Index *index);


#endif

