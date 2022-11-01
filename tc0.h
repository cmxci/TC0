#define N	8
#define R	64
#define K	(N * N * N * N)

#define ROTL(A, n) ((A) << n) | ((A) >> (32 - n))

extern const unsigned char tc0_default_sbox[256];

char* tc0_encrypt(char* src, size_t len, unsigned char* key, const unsigned char* sbox);

char* tc0_decrypt(char* src, size_t len, unsigned char* key, const unsigned char* sbox);

size_t next(size_t n, unsigned long m);
