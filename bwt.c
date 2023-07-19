#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

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
	// If curChar is the alphabetically largest, return the last index of the array
	return index->count;
}

int rank(struct Index *index, char curChar, int line) {
	return index->occ[line - 1][(int)curChar];	
}

char getBwtChar(struct Index *index, int offset) {
	return index->source[offset - 1];
}

// Backwards reconstrution functions

void reverseStr(char* str) {
    int length = strlen(str);
    int i, j;
	char temp;

    for (i = 0, j = length - 1; i < j; i++, j--) {
        temp = str[i];
        str[i] = str[j];
        str[j] = temp;
    }
}

/*
 *	Performs a BWT reverse to find the record that contains the search string
 */
long getNextRecord(struct Index *index, int offset) {
	char curChar = getBwtChar(index, offset);
	char *record = NULL;
	// Find the start of the record
	while (offset > 0 && curChar != ']') {
		offset = rank(index, curChar, offset) + index->c[(int)curChar];
		curChar = getBwtChar(index, offset);
	}

	// Find the record
	while (offset > 0 && curChar != '[') {
		offset = rank(index, curChar, offset) + index->c[(int)curChar];
		curChar = getBwtChar(index, offset);

		// Append to record string
		if (curChar != '[') {
			if (record == NULL) {
				record = (char *)realloc(record, sizeof(char)+1);
				record[0] = curChar;
				record[1] = '\0';
			} else {
				record = (char *)realloc(record, strlen(record) + 1);
				record = strncat(record, &curChar, 1);
			}
		}
	}
	reverseStr(record);
	char *endptr;

	long nextRecord = strtol(record, &endptr, 10) + 1;
	free(record);
	return nextRecord;
}

/*
 *	Performs a backwards search for a given pattern
 *	returns 1 on success, 0 on failure
 */
int searchBWT(struct Index *index, char *pattern, int *first, int *last) {
	int pIndex = strlen(pattern);
	char curChar = pattern[pIndex-1];
	*first = index->c[(int)curChar] + 1;
	*last = getNextC(index, curChar);

	// Consider the case where the last record is matched

	// Pattern not found
	if (*first == 1) {
		return 0;
	}

	while (*first <= *last && pIndex >= 2) {
		curChar = pattern[pIndex-2];
		*first = index->c[(int)curChar] + rank(index, curChar, *first - 1) + 1;
		*last = index->c[(int)curChar] + rank(index, curChar, *last);
		pIndex--;
	}

	/* printf("first: %d, last: %d\n", *first, *last); */
	if (*last < *first) {
		return 0;
	}
	// Calculate matches based on the suffixes that match the pattern
	/* if (*last < *first) { */
	/* 	printf("no matches\n"); */
	/* } else { */
	/* 	printf("MATCH FOUND - %d to %d\n", *first, *last); */
	/* } */
	return 1;
}

/*
 *	Given the suffix index, returns a string containing all chars from the index to ']'
 */
char *extractStr(struct Index *index, int offset) {
	char curChar = getBwtChar(index, offset);
	char *record;
	while (offset > 0 && curChar != ']') {
		if (record == NULL) {
			record = (char *)realloc(record, sizeof(char)+1);
			record[0] = curChar;
			record[1] = '\0';
		} else {
			record = (char *)realloc(record, strlen(record) + 1);
			record = strncat(record, &curChar, 1);
		}
		offset = rank(index, curChar, offset) + index->c[(int)curChar];
		curChar = getBwtChar(index, offset);
	}
	reverseStr(record);
	return record;
}

/*
 *	Performs the actual BWT search on an indexed BWT string
 */
struct MatchList *findMatches(struct Index *index, struct MatchList *matches, char *pattern) {
	// Find the consecutive suffixes of all matches <first,last>
	int first;
	int last;
	searchBWT(index, pattern, &first, &last);

	/* for (int i = 0; i < ALPHABET_SIZE ; i++) { */
	/* 	printf("|char: %c|value: %d|\n", (char)i, index->c[i]); */
	/* } */

	// Find the record associated with each match, find the next consective record
	// and reconstruct the entire record.
	long record;
	char *recordStr = (char *)malloc(sizeof(char)*30);
	int start;
	int end;
	struct Match *curMatch;
	while (first <= last) {
		record = getNextRecord(index, first);
		sprintf(recordStr, "[%ld", record);
		/* printf("%s\n",recordStr); */

		if (searchBWT(index, recordStr, &start, &end) == 0) {
			// Handle last record match
			searchBWT(index, "[", &start, &end);
		}
		/* if (start != end) { */
		/* 	printf("multiple record matches???? exiting..."); */
		/* 	exit(1); */
		/* } */
		curMatch = initMatch(record-1,  extractStr(index, start));
		addMatch(matches, curMatch);

		first++;
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
	newMatch->next = NULL;
	if (newMatch->string == NULL) {
		printf("UNABLE TO ALLOCATE STRING FOR MATCH\n");
		exit(1);
	}
	return newMatch;
}

void addMatch(struct MatchList *matchList, struct Match *match) {
	struct Match *prev = NULL;
	struct Match *cur = matchList->head;	
	if (cur == NULL) {
		matchList->head = match;
	} else {
		while (cur != NULL) {
			if (cur->record == match->record) {
				break;
			}
			if (cur->record > match->record) {
				if (prev == NULL) {
					matchList->head = match;
					match->next = cur;
					break;
				} else {
					prev->next = match;
					match->next = cur;
					break;
				}

			}
			prev = cur;
			cur = cur->next;

		}
		if (cur == NULL) {
			prev->next = match;
		}
	}
	
}

void printMatches(struct MatchList *matchList) {
	struct Match *cur = matchList->head;
	if (cur == NULL) {
		printf("No matches");
	} else {
		while (cur != NULL) {
			printf("[%d]%s\n", cur->record, cur->string);
			cur=cur->next;
		}
	}
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
