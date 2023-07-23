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

int getGapOffset(struct Index *index, int offset) {
	int gap = index->gapSize;
	if (gap <= 0) {
		printf("Invalid gap - unable to calc offset\n");
		exit(1);
	}
	return (offset-(offset%gap))/gap;
}
	

/*
 *	Given an offset into L, returns the occurrence array at that character
 *	(Reads up to n characters, used for smaller files)
 */
int getOccAtOffsetSlow(struct Index *index, int offset, char curChar) {
	int count = -1;
	int *curOcc = (int *)calloc(ALPHABET_SIZE, sizeof(int));

	// Continually read each count from rlb file
	while (count < offset) { 
		count++;
		curOcc[(int)index->source[count]]++;
	}
	
	int result = curOcc[(int)curChar];
	free(curOcc);
	return result;
}

/*
 * Given an occurrence object that corresponds to the beginning of a gap,
 * traverses the rlb from the rlbPos in the occurrence object to the given offset,
 * updating the occ array and returning the character at offset.
 */
char getOccFromRlb(struct Args *args, struct Index *index, struct Occ *curOcc, int blockOffset, int lastOffset) {
	long posCount = 0;
	int runLen = 0 - curOcc->curRunCount;
	int curRlbPos = curOcc->rlbPos;
	int curLPos = blockOffset*index->gapSize; 
	char *run = (char *)malloc(sizeof(char));
	run[0] = '\0';

	fseek(args->rlbFile, curRlbPos, SEEK_SET);

	char prev = '\0';
	int a = curLPos;
	int b = lastOffset;

	while(getNextRun(args, run, &posCount, &posCount) && curLPos < lastOffset) {
		runLen += runToInt(run, 0, strlen(run)-1);
		while(runLen > 0 && curLPos < lastOffset) {
			curOcc->occ[(int)run[0]]++;
			runLen--;
			curLPos++;
		}
		prev = run[0];	
		runLen = 0;
	}
	char curChar = run[0];
	free(run);

	// If the offset into the text you are looking for aligns with a gap start
	// then return the char associated with that occ
	if (a != b) {
		return prev;
	} else {
		return curChar;
	}
}

int rank(struct Args *args, struct Index *index, char curChar, int line) {
	int a = line - 1;
	if (a < 0) {
		return -1;
	}

	if (index->rlbSize <= SMALL_FILE_MAX) {
		return getOccAtOffsetSlow(index, a, curChar);	
	} else {
		int blockOffset = getGapOffset(index, a);
		struct Occ *curOcc = (struct Occ *)malloc(sizeof(struct Occ));

		fseek(args->indexFile, blockOffset*sizeof(struct Occ), SEEK_SET);
		fread(curOcc, sizeof(struct Occ), 1, args->indexFile);
		
		if (a % index->gapSize != 0) {
			getOccFromRlb(args, index, curOcc, blockOffset, a);
		}
		int result = curOcc->occ[(int)curChar];
		free(curOcc);
		return result;
	}
}

char getBwtCharSlow(struct Args *args, struct Index *index, int offset) {
	int a = offset - 1;
	return index->source[a];
}

char getBwtChar(struct Args *args, struct Index *index, int offset) {
	int a = offset - 1;

	if (index->rlbSize <= SMALL_FILE_MAX) {
		return index->source[a];
	} else {
		int blockOffset = getGapOffset(index, a);

		struct Occ *curOcc = (struct Occ *)malloc(sizeof(struct Occ));
		fseek(args->indexFile, blockOffset*sizeof(struct Occ), SEEK_SET);
		fread(curOcc, sizeof(struct Occ), 1, args->indexFile);
		
		char result = getOccFromRlb(args, index, curOcc, blockOffset, offset - 1); 
		free(curOcc);
		return result;
	}
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
long getNextRecord(struct Args *args, struct Index *index, int offset) {
	char curChar = getBwtChar(args, index, offset);
	char *record = NULL;
	// Find the start of the record
	while (offset > 0 && curChar != ']') {
		offset = rank(args, index, curChar, offset) + index->c[(int)curChar];
		curChar = getBwtChar(args, index, offset);
	}


	// Find the record
	while (offset > 0 && curChar != '[') {
		offset = rank(args, index, curChar, offset) + index->c[(int)curChar];
		curChar = getBwtChar(args, index, offset);

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
char *extractStr(struct Args *args, struct Index *index, int offset) {
	char curChar = getBwtChar(args, index, offset);
	char *record = (char *)malloc(sizeof(char));;
	record[0] = '\0';
	while (offset > 0 && curChar != ']') {
		// Add current decoded char to ouput string
		record = (char *)realloc(record, strlen(record) + 2);
		record = strncat(record, &curChar, 1);

		offset = rank(args, index, curChar, offset) + index->c[(int)curChar];
		curChar = getBwtChar(args, index, offset);
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
int searchBWT(struct Args *args, struct Index *index, char *pattern, int *first, int *last) {
	int pIndex = strlen(pattern);
	char curChar = pattern[pIndex-1];
	*first = index->c[(int)curChar] + 1;
	*last = getNextC(index, curChar);

	// Pattern not found
	if (*first == 1) {
		return 0;
	}

	while (*first <= *last && pIndex >= 2) {
		curChar = pattern[pIndex-2];
		if (rank(args, index, curChar, *first - 1) == -1) {
			return 2;
		}
		*first = index->c[(int)curChar] + rank(args, index, curChar, *first - 1) + 1;
		*last = index->c[(int)curChar] + rank(args, index, curChar, *last);
		pIndex--;
	}

	if (*last < *first) {
		return 0;
	}

	return 1;
}

/*
 * Given a record number, checks if the record has already been found
 */
int recordAlreadyMatched(struct MatchList *matches, long record) {
	struct Match *cur = matches->head;
	int found = 0;
	while (cur != NULL) {
		if (cur->record == record) {
			return 1;
		}
		cur = cur->next;
	}
	return found;
}


/*
 *	Performs the actual BWT search on an indexed BWT string
 */
struct MatchList *findMatches(struct Args *args, struct Index *index, struct MatchList *matches, char *pattern) {
	// Find the consecutive suffixes of all matches <first,last>
	int first;
	int last;
	if (searchBWT(args, index, pattern, &first, &last) == 0) {
		first = last+1;
	}

	// Find the record associated with each match, find the next consective record
	// and reconstruct the entire record.
	long record;
	char *recordStr = (char *)malloc(sizeof(char)*30);
	int start;
	int end;
	struct Match *curMatch;
	int result;
	while (first <= last) {
		record = getNextRecord(args, index, first);
		if (recordAlreadyMatched(matches, record-1)) {
			first++;
			continue;
		}
		/* printf("record - %ld\n", record-1); */
		sprintf(recordStr, "[%ld]", record);

		if (searchBWT(args, index, recordStr, &start, &end) == 0) {
			// Handle last record match
			//  - Counts up from 0 looking for the first record
			for (long i = 0 ; i <= record - 1 ; i++) {
				sprintf(recordStr, "[%ld]", i);

				result = searchBWT(args, index, recordStr, &start, &end);
				if (result == 1) {
					break;
				} else if (result == 2) {
					// handle case where first index is first record
					sprintf(recordStr, "%ld]", i);
					searchBWT(args, index, recordStr, &start, &end);
					break;
				}
			}
		}
		char *strMatch = extractStr(args, index, start); 
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

