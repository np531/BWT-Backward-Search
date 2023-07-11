#ifndef BWT_H
#define BWT_H

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


#endif

