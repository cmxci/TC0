#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include "encryption.h"

struct layer {
	unsigned char d[N][N];
};

struct cube {
	struct layer l[N];
};

struct state {
	struct cube c[N];
	struct state* prev;
	unsigned char initial;
};

const unsigned char tc0_default_sbox[256] = {
	/* 0     1    2      3     4     5     6     7     8     9     A     B     C     D     E     F */
	0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
	0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
	0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
	0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
	0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
	0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
	0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
	0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
	0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
	0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
	0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
	0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
	0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
	0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
	0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
	0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16 };

unsigned char* gen_rsbox(const unsigned char* sbox) {
	unsigned char* rsbox = malloc(256);
	for (unsigned short i = 0; i < 256; i++) {
		rsbox[sbox[i]] = i;
	}
	return rsbox;
}

void layer_sub(struct layer* l, const unsigned char* sbox) {
	for (unsigned long i = 0; i < N; i++) {
		for (unsigned long j = 0; j < N; j++) {
			(*l).d[i][j] = sbox[(*l).d[i][j]];
		}
	}
}

void cube_sub(struct cube* s, const unsigned char* sbox) {
	for (unsigned int i = 0; i < N; i++) {
		layer_sub(&((*s).l[i]), sbox);
	}
}

void swap(unsigned char* x, unsigned char* y) {
	unsigned char t = *x;
	*x = *y;
	*y = t;
}

void rotate_layer(struct layer* l) {
	for(unsigned long i = 0; i < N; i++) {
		for(unsigned long j = 0; j < i; j++) {
			swap(&((*l).d[i][j]), &((*l).d[j][i]));
		}
	}
}

unsigned char rot_counts[4] = {0, 1, 2, 3};
unsigned char reverse_rot_counts[4] = {0, 3, 2, 1};

void rotate_layers(struct cube* s) {
	for (unsigned long i = 0; i < N; i++) {
		for (unsigned char j = 0; j < rot_counts[i % 4]; j++) {
			rotate_layer(&((*s).l[i]));
		}
	}
}

void unrotate_layers(struct cube* s) {
	for (unsigned long i = 0; i < N; i++) {
		for (unsigned char j = 0; j < reverse_rot_counts[i % 4]; j++) {
			rotate_layer(&((*s).l[i]));
		}
	}
}

void xor_layer(struct layer* dst, struct layer* src) {
	for (unsigned long i = 0; i < N; i++) {
		for (unsigned long j = 0; j < N; j++) {
			(*dst).d[i][j] ^= (*src).d[i][j];
		}
	}
}

void memxor(void* dst, void* src, size_t size) {
	for (size_t i = 0; i < size; i++) {
		((char*)dst)[i] ^= ((char*)src)[i];
	}
}

void mix_cube(struct cube* s) {
	unsigned long i;
	for (i = 0; i < N - 1; i++) {
		xor_layer(&((*s).l[i]), &((*s).l[i + 1]));
	}
	for (i = N - 1; i > 0; i--) {
		xor_layer(&((*s).l[i]), &((*s).l[i - 1]));
	}
}

void unmix_cube(struct cube* s) {
	unsigned long i;
	for (i = 1; i < N; i++) {
		xor_layer(&((*s).l[i]), &((*s).l[i - 1]));
	}
	for (i = N - 2; i >= 0 && i < N; i--) {
		xor_layer(&((*s).l[i]), &((*s).l[i + 1]));
	}
}

void byte_mix(struct cube* s, unsigned short k) {
	for (unsigned long i = 0; i < (N * N * N); i++) {
		unsigned long j = (i * k) % ((N * N * N) - 1);
		swap(&((*s).l[i / (N * N)].d[(i / N) % N][i % N]), &((*s).l[j / (N * N)].d[(j / N) % N][j % N]));
	}
}

void byte_unmix(struct cube* s, unsigned short k) {
	for (unsigned long i = (N * N * N) - 1; i > 0; i--) {
		unsigned long j = (i * k) % ((N * N * N) - 1);
		swap(&((*s).l[i / (N * N)].d[(i / N) % N][i % N]), &((*s).l[j / (N * N)].d[(j / N) % N][j % N]));
	}
}

void mix_cubes(struct state* s) {
	unsigned long i = 0;
	for (i = 0; i < N; i++) {
		mix_cube(&((*s).c[i]));
	}
	for (i = 0; i < N - 1; i++) {
		memxor(&((*s).c[i]), &((*s).c[i + 1]), sizeof(struct cube));
	}
	for (i = N - 1; i > 0; i--) {
		memxor(&((*s).c[i]), &((*s).c[i - 1]), sizeof(struct cube));
	}
}

void unmix_cubes(struct state* s) {
	unsigned long i = 0;
	for (i = 0; i < N; i++) {
		unmix_cube(&((*s).c[i]));
	}
	for (i = 1; i < N; i++) {
		memxor(&((*s).c[i]), &((*s).c[i - 1]), sizeof(struct cube));
	}
	for (i = N - 2; i >= 0 && i < N; i--) {
		memxor(&((*s).c[i]), &((*s).c[i + 1]), sizeof(struct cube));
	}
}

void round_encrypt_state(struct state* s, const unsigned char* sbox, unsigned char* rk) {
	for (unsigned long i = 0; i < N; i++) {
		cube_sub(&((*s).c[i]), sbox);
		rotate_layers(&((*s).c[i]));
		byte_mix(&((*s).c[i]), 32);
		byte_mix(&((*s).c[i]), 16);
		byte_mix(&((*s).c[i]), 2);
	}
	mix_cubes(s);
	memxor((*s).c, rk, K);
	if ((*s).initial) return;
	memxor((*s).c, (*(*s).prev).c, K);
}

void round_decrypt_state(struct state* s, const unsigned char* rsbox, unsigned char* rk) {
	if (!(*s).initial) {
		memxor((*s).c, (*(*s).prev).c, K);
	}
	memxor((*s).c, rk, K);
	unmix_cubes(s);
	for (unsigned long i = 0; i < N; i++) {
		byte_unmix(&((*s).c[i]), 2);
		byte_unmix(&((*s).c[i]), 16);
		byte_unmix(&((*s).c[i]), 32);
		unrotate_layers(&((*s).c[i]));
		cube_sub(&((*s).c[i]), rsbox);
	}
}

void round_key(unsigned char* dst, unsigned char* key, unsigned long round) {
	memcpy(dst, key, K);
	for (unsigned long i = 0; i < K; i++) {
		dst[i] = ROTL(dst[i] ^ dst[(i + 1) % K], ((round * ROTL((uint32_t)((key[i] << 24) & (key[(i + 1) % K] << 16) & (key[(i + 2) % K] << 16) & (key[(i + 3) % K])), i % 32)) ^ 0x9E3779B9) % 0xFF);
	}
}

size_t next(size_t n, unsigned long m) {
	unsigned long r = n % m;
	if (r == 0) return n;
	return n + m - r;
}

char* tc0_encrypt_sb(char* src, size_t len, unsigned char* key, unsigned char* sbox) {
	char* dst = calloc(next(len, K), 1);
	unsigned long blocks = next(strlen(src), K) / K;
	char* next = dst;
	struct state* prev = NULL;
	for (unsigned long i = 0; i < blocks; i++) {
		struct state* s = malloc(sizeof(struct state));
		memcpy((*s).c, src, K);
		src += K;
		if (!i) (*s).initial = 1;
		else (*s).initial = 0;
		(*s).prev = prev;
		unsigned char* rk = malloc(K);
		for (unsigned long j = 0; j < R; j++) {
			round_key(rk, key, j);
			round_encrypt_state(s, sbox, rk);
		}
		free(rk);
		memcpy(next, (*s).c, K);
		next += K;
		free(prev);
		prev = s;
	}
	free(prev);
	return dst;
}

char* tc0_decrypt_sb(char* src, size_t len, unsigned char* key, unsigned char* sbox) {
	char* dst = calloc(next(len, K), 1);
	unsigned long blocks = next(strlen(src), K) / K;
	char* wnext = dst + ((blocks - 1) * K);
	char* rnext = src + ((blocks - 1) * K);
	unsigned char* rsbox = gen_rsbox(sbox);
	for (unsigned long i = 0; i < blocks; i++) {
		struct state s;
		memcpy(s.c, rnext, K);
		rnext -= K;
		if (i == blocks - 1) s.initial = 1;
		else {
			struct state p;
			memcpy(p.c, rnext, K);
			s.prev = &p;
			s.initial = 0;
		}
		unsigned char* rk = malloc(K);
		for (unsigned long j = R - 1; j >= 0 && j < R; j--) {
			round_key(rk, key, j);
			round_decrypt_state(&s, rsbox, rk);
		}
		free(rk);
		memcpy(wnext, s.c, K);
		wnext -= K;
	}
	free(rsbox);
	return dst;
}

char* tc0_encrypt(char* src, unsigned char* key) {
	return tc0_encrypt_sb(src, key, tc0_default_sbox);
}

char* tc0_decrypt(char* src, unsigned char* key) {
	return tc0_decrypt_sb(src, key, tc0_default_sbox);
}
