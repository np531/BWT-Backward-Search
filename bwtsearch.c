#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <inttypes.h>
#include <stdbool.h>

#define PATTERN_MAX 512
extern char* strdup(const char*);

struct Args {
	FILE *rlbFile;
	FILE *indexFile;
	char *pattern;
};

struct Args *parseArgs(int argc, char **argv) {
	if (argc != 4) {
		printf("Incorrect number of arguments provided");
		exit(1);
	}

	struct Args *args = (struct Args *)malloc(sizeof(struct Args));
	if (args == NULL) {
		printf("Memory allocation error when initialising arg container");
		exit(1);
	}

	args->pattern = strdup(argv[3]);
	args->rlbFile = fopen(argv[1], "rb");
	if (args->rlbFile == NULL) {
		printf("rlb file does not exist");
		exit(1);
	}
	args->indexFile = fopen(argv[2], "w+");

	return args;
}

void freeArgs(struct Args *args) {
	free(args->pattern);
	free(args);
}


int main(int argc, char **argv) {
	struct Args *args = parseArgs(argc, argv);
	printf("%s", args->pattern);
	freeArgs(args);

	return 0;
}

