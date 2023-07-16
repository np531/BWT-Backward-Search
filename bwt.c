#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <stdbool.h>

#include "bwt.h"
#include "bwtsearch.h"

struct MatchList *searchBWT(struct MatchList *matches, char *source, char *pattern) {
	printf("%s\n", source);
	return matches;
}

void buildC(struct Index *index) {
	printf("\n");
	printf("%s\n", index->source);

	for (int i = 0 ; i < strlen(index->source) ; i++) {
		index->c[(int)index->source[i]]++;
	}
}
