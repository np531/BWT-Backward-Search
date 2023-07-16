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

void buildTables(struct Index *index) {
	int j = 0;
	int *prev = index->occ[0];
	// Construct the C table
	for (int i = 0 ; i < strlen(index->source) ; i++) {
		// Update C array
		j = (int)index->source[i];
		j++;
		while (j < ALPHABET_SIZE) {
			index->c[j]++;
			j++;
		}

		// Update the Occ table
		if (i == 0) {
			index->occ[i][(int)index->source[i]]++;
		} else {
			index->occ = (int **)realloc(index->occ, (i+1)*sizeof(int *));
			if (index->occ == NULL) {
				printf("Unable to allocate memory for occ\n");
				exit(1);
			}

			index->occ[i] = (int *)calloc(ALPHABET_SIZE, sizeof(int));
			if (index->occ[i] == NULL) {
				printf("Unable to allocate memory for occ\n");
				exit(1);
			}
			memcpy(index->occ[i], prev, ALPHABET_SIZE*sizeof(int));
			index->occ[i][(int)index->source[i]]++;
			prev = index->occ[i];
		}
	}
	/* for (int i = 0 ; i < ALPHABET_SIZE ; i++) { */
	/* 	printf("%c, %d\n", (char)i, index->occ[11][i]); */
	/* } */
	/* printf("%d\n", 0%16); */
	
}
