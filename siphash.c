#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <stdbool.h>
#include <time.h>
#include <omp.h>

#define RAND_MAX (2<<32 - 1)
#define PARALLEL 1
#define ROT(x, b) (uint64_t)(((x) << (b)) | ((x) >> (64-(b))))

// constant
const uint64_t init[4] = {0x736f6d6570736575, 0x646f72616e646f6d, 0x6c7967656e657261, 0x7465646279746573};
const int c=2;
const int d=4;
// v is global for the readability of the code
uint64_t v[4];
// args
uint64_t k[2];
uint64_t k2[2];
// uint8_t m[] = {0x0, 0x1, 0x3, 0x4, 0x5, 0x6, 0x7};
uint8_t m[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e};
uint8_t m2[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
unsigned mlen;


void sip_round() {
    // para
    v[0] += v[1];
    v[1] = ROT(v[1], 13);
    v[1] ^= v[0];
    v[0] = ROT(v[0],32);
    // para
    v[2] += v[3];
    v[3] = ROT(v[3], 16);
    v[3] ^= v[2];
    // para
    v[2] += v[1];
    v[1] = ROT(v[1], 17);
    v[1] ^= v[2];
    v[2] = ROT(v[2], 32);
    // para
    v[0] += v[3];
    v[3] = ROT(v[3], 21);
    v[3] ^= v[0];
}


uint64_t sip_hash_2_4(uint64_t k[2], uint8_t *m, unsigned mlen) {
    uint64_t result;
    uint8_t w = (mlen + 1)%8 ? (mlen + 1)/8 + 1 : (mlen + 1) / 8;
    uint64_t words[w];
    int i;
    int j;

    // initialization
    for (i = 0; i<4; i++) {
        v[i] = k[i%2] ^ init[i];
    }

    // compression
    for (i = 0; i<w-1; i++) {
        words[i] = 0;
        for (j = 0; j<8; j++) { // each word is 8 bytes long
            words[i] += ((uint64_t) m[8*i+j] << 8*j);
        }
        v[3] = v[3] ^ words[i];
        for (int k = 0; k<c; k++) {
            sip_round();
        }
        v[0] = v[0] ^ words[i];
    }
    words[w-1] = 0;
    for (j = 0; j<(mlen%8); j++) {
        words[w-1] += ((uint64_t) m[8*(w-1)+j] << 8*j);
    }
    // the last cell of words is filled here:
    words[w-1] += ((uint64_t) mlen%256) << 56;
    v[3] = v[3] ^ words[w-1];
    // c siprounds
    for (i = 0; i<c; i++) {
        sip_round();
    }
    v[0] = v[0] ^ words[w-1];

    // Finalization
    v[2] = v[2] ^ 0xff;
    for (i = 0; i<d; i++) {
        sip_round();
    }

    result = v[0] ^ v[1] ^ v [2] ^ v[3];

    return result;
}

uint32_t sip_hash_fix32(uint32_t k, uint32_t m) {
    uint64_t kp[2] = {0, (uint64_t) k};
    uint8_t *mp = calloc(4, sizeof(uint8_t));
    unsigned mplen = 4;

    for (int i = 0; i<4; i++) {
        mp[i] = (uint8_t) m>>(i*8); // TODO double check this
    }

    return (uint32_t) sip_hash_2_4(kp, mp, mplen);
}

bool array_search(uint32_t *array, uint32_t len, uint32_t value) {
    bool found = false;
    register int i;

#if PARALLEL
    #pragma omp parallel for schedule(static) num_threads(8) shared(array, len, value) private(i) reduction(||:found)
#endif
    for (i=0; i<len; i++) {
        // printf("array_search with %i threads\n", omp_get_num_threads());
        found = found || (array[i] == value);
    }

    return found;
}

uint64_t coll_search(uint32_t k, uint32_t (*fun)(uint32_t, uint32_t)) {
    uint32_t id_collision = 0;
    // we can reduce the size of this array sqrt (BParadoxe)

    uint32_t max_expected_size = 1<<16-1;
    uint32_t *bon_array = calloc(max_expected_size, sizeof(uint32_t));
    srand(time(NULL));
    uint32_t m = rand();
    uint32_t result = sip_hash_fix32(k, m);

    while (!array_search(bon_array, id_collision, result)) {
        bon_array[id_collision] = result;
        id_collision++;
        printf("%i\n", id_collision);
        m = rand();
        result = sip_hash_fix32(k, m);
    }

    free(bon_array);

    return id_collision;
}

void main(int *argc, char **argv) {
    mlen = 8;
    // k[0] = 0x0706050403020100;
    // k[1] = 0x0f0e0d0c0b0a0908;
    // k2[0] = 0x0;
    // k2[1] = 0x0;
    // k[0] = 0;
    // k[1] = 0;
    uint32_t k = 0;

    // printf("siphash_2_4(k, m, mlen) = %p\n", sip_hash_2_4(k, m2, mlen));
    printf("collision found after %i tries\n", coll_search(k, sip_hash_fix32));

    return;
}
