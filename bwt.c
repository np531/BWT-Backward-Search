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
	int a = line - 1;
	/* int b = index->count; */
	if (line - 1 < 0) {
		/* printf("a\n"); */
		return -1;
		/* printf("encountered negative index - %d\n", (a%b)+b); */
		/* a = 3; */
		/* exit(1); */
	}
	return index->occ[a][(int)curChar];	
}

char getBwtChar(struct Index *index, int offset) {
	if (offset - 1 < 0) {
		/* printf("encountered negative offset\n"); */
	}
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
 *	Given the suffix index, returns a string containing all chars from the index to ']'
 */
char *extractStr(struct Index *index, int offset) {
	char curChar = getBwtChar(index, offset);
	char *record = (char *)malloc(sizeof(char));;
	record[0] = '\0';
	/* printf("\na\n"); */
	while (offset > 0 && curChar != ']') {
		// Add current decoded char to ouput string
		record = (char *)realloc(record, strlen(record) + 2);
		record = strncat(record, &curChar, 1);

		offset = rank(index, curChar, offset) + index->c[(int)curChar];
		curChar = getBwtChar(index, offset);
	}

	reverseStr(record);
	// Removes previous record junk from the end of the record (occurs on last record match sometimes)
	if (strlen(record) >= 2 && record[strlen(record)-2] == '[') {
		record[strlen(record)-2] = '\0';
	} else if (strlen(record) >= 1 && record[strlen(record)-1] == '[') {
		record[strlen(record)-1] = '\0';
	} 
	return record;
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
		/* printf("\nfirst - |%d|\n", *first); */
		if (rank(index, curChar, *first - 1) == -1) {
			return 2;
		}
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
 *	Performs the actual BWT search on an indexed BWT string
 */
struct MatchList *findMatches(struct Index *index, struct MatchList *matches, char *pattern) {
	// Find the consecutive suffixes of all matches <first,last>
	int first;
	int last;
	if (searchBWT(index, pattern, &first, &last) == 0) {
		first = last+1;
	}

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
	int result;
	while (first <= last) {
		record = getNextRecord(index, first);
		sprintf(recordStr, "[%ld]", record);
		/* printf("%s\n",recordStr); */

		if (searchBWT(index, recordStr, &start, &end) == 0) {
			// Handle last record match
			//  - Counts up from 0 looking for the first record
			for (long i = 0 ; i <= record - 1 ; i++) {
				sprintf(recordStr, "[%ld]", i);
				/* printf("[%ld]\n", i); */

				result = searchBWT(index, recordStr, &start, &end);
				if (result == 1) {
					break;
				} else if (result == 2) {
					// handle case where first index is first record
					sprintf(recordStr, "%ld]", i);
					searchBWT(index, recordStr, &start, &end);
					break;
				}
			}
		}
		char *strMatch = extractStr(index, start); 
		curMatch = initMatch(record-1,  strMatch);
		free(strMatch);
		addMatch(matches, curMatch);

		first++;
	}
	free(recordStr);

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
		// TODO - remove me
		printf("No matches\n");
	} else {
		while (cur != NULL) {
			printf("[%d]%s\n", cur->record, cur->string);
			cur=cur->next;
		}
	}
}

