#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <inttypes.h>
#include <stdbool.h>
#include "bwtsearch.h"
#include "bwt.h"

int main(int argc, char **argv) {
	struct Args *args = parseArgs(argc, argv);
	printf("Pattern - %s\n", args->pattern);
	char *source = parseBWTString(args);

	struct MatchList *matches = initMatchList();
	matches = searchBWT(matches, source, args->pattern);
	printf("%d\n", matches->size);

	freeArgs(args);
	return 0;
}

struct MatchList *initMatchList() {
	struct MatchList *newMatchList = (struct MatchList *)malloc(sizeof(struct MatchList));
	newMatchList->matches = NULL;
	newMatchList->size = 0;
	return newMatchList;
}

struct Args *parseArgs(int argc, char **argv) {
	if (argc != 4) {
		printf("Incorrect number of arguments provided\n");
		exit(1);
	}

	struct Args *args = (struct Args *)malloc(sizeof(struct Args));
	if (args == NULL) {
		printf("Memory allocation error when initialising arg container\n");
		exit(1);
	}

	args->pattern = strdup(argv[3]);
	args->rlbFile = fopen(argv[1], "rb");
	if (args->rlbFile == NULL) {
		printf("rlb file does not exist\n");
		exit(1);
	}
	args->indexFile = fopen(argv[2], "w+");

	return args;
}

void freeArgs(struct Args *args) {
	fclose(args->rlbFile);
	fclose(args->indexFile);
	free(args->pattern);
	free(args);
}


char *parseBWTString(struct Args *args) {
	fseek(args->rlbFile, 0, SEEK_END);
	long fileSize = ftell(args->rlbFile);
	char *source = (char *)malloc((fileSize+1)*sizeof(char));

	fseek(args->rlbFile, 0, SEEK_SET);
	long sourceSize = fread(source, sizeof(char), fileSize, args->rlbFile);
	source[sourceSize] = '\0';
	return source;
}
