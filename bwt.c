#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <stdbool.h>

#include "bwt.h"
#include "bwtsearch.h"

// Index helper functions
int getNextC(struct Index *index, char curChar) {
	int i = (int)curChar + 1;
	while (i < ALPHABET_SIZE) {
		if (index->c[i] != 0) {
			return index->c[i];		
		}
		i++;
	}
	return index->count;
}

int rank(struct Index *index, char curChar, int line) {
	return index->occ[line - 1][(int)curChar];	
}

/*
 *	Performs the actual BWT search on an indexed BWT string
 */
struct MatchList *searchBWT(struct Index *index, struct MatchList *matches, char *pattern) {
	/* for (int i = 0 ; i < ALPHABET_SIZE ; i++) { */
	/* 	printf("char: %c, num: %d\n", (char)i, index->c[i]); */
	/* } */
	/* Start: */

	// Backwards Search Algorithm
	int pIndex = strlen(pattern);
	char curChar = pattern[pIndex-1];
	int first = index->c[(int)curChar] + 1;
	int last = getNextC(index, curChar);

	while (first <= last && pIndex >= 2) {
		printf("%c\n", curChar);
		curChar = pattern[pIndex-2];
		first = index->c[(int)curChar] + rank(index, curChar, first - 1) + 1;
		last = index->c[(int)curChar] + rank(index, curChar, last);
		printf("%c, %d, %d\n", curChar, first, last);
		pIndex--;
	}

	if (last < first) {
		printf("no matches\n");
	} else {
		printf("MATCHES FOUND - %d to %d", first, last);
	}

	return matches;
}


/*
 *	Initialises a single string match object
 */
struct Match *initMatch(int record, char *string) {
	struct Match *newMatch = (struct Match *)malloc(sizeof(struct Match));
	newMatch->record = record;
	newMatch->string = strdup(string);
	if (newMatch->string == NULL) {
		printf("UNABLE TO ALLOCATE STRING FOR MATCH\n");
			exit(1);
	}
	return newMatch;
}

/*
 *	Builds the occurrence and C tables used for indexing the BWT string
 */
void buildTables(struct Index *index) {
	int j = 0;
	int *prev = index->occ[0];
	// Construct the occ table
	for (int i = 0 ; i < strlen(index->source) ; i++) {
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
	
	// Construct the C array
	int *finalOcc = (int *)malloc(sizeof(int)*127);
	memcpy(finalOcc, index->occ[index->count-1], sizeof(int)*127);
	for (int i = ALPHABET_SIZE - 1 ; i >= 0 ; i--) {
		if (finalOcc[i] != 0) {
			j = i - 1;
			while (j >= 0) {
				index->c[i] += finalOcc[j];
				j--;
			}
		}
	}
	free(finalOcc);
}
