#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "tc0.h"

#define DECRYPT (0x01)
#define USE_LEN_HEADER (0x02)

int main(int argc, char** argv) {
	char flags = 0x0;
	char* text = malloc(K);
	char* result;
	char* key = malloc(K);
	uint64_t cur_sz = 0, max_sz = K, len = 0;
	for (unsigned long i = 0; i < argc; i++) {
		if (argv[i][0] == '-') {
			if (strchr(argv[i], 'd')) flags |= DECRYPT;
			if (strchr(argv[i], 'h')) flags |= USE_LEN_HEADER;
		}
		else {
			FILE* kf = fopen(argv[i], "r");
			fread(key, 1, K, kf);
			fclose(kf);
		}
	}

	if ((flags & USE_LEN_HEADER) && (flags & DECRYPT)) {
		fread(&len, sizeof(uint64_t), 1, stdin);
		max_sz = next(len, K);
		text = realloc(text, max_sz);
	}
	if (len > 0) fread(text, 1, len, stdin);
	else {
		char c;
		while ((c = fgetc(stdin) != EOF)) {
			if (cur_sz + 1 >= max_sz) max_sz += K;
			text = realloc(text, max_sz);
			cur_sz++;
			text[cur_sz] = c;
		}
	}
	if (flags & DECRYPT) result = tc0_decrypt(text, cur_sz, key, tc0_default_sbox);
	else result = tc0_encrypt(text, cur_sz, key, tc0_default_sbox);
	if ((flags & USE_LEN_HEADER) && !(flags & DECRYPT)) fwrite(&cur_sz, sizeof(uint64_t), 1, stdout);
	if (len > 0 && (flags & DECRYPT)) fwrite(result, sizeof(char), len, stdout);
	else fwrite(result, sizeof(char), max_sz, stdout);
}
