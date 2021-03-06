#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <stdbool.h>
#include <time.h>

// already defined with this value:
// #define RAND_MAX (2<<32 - 1)
#define ROT(x, b) (uint64_t)(((x) << (b)) | ((x) >> (64-(b))))

// constant
const uint64_t init[4] = {0x736f6d6570736575, 0x646f72616e646f6d, 0x6c7967656e657261, 0x7465646279746573};
const int c=2;
const int d=4;
// v is global for the readability of the code
uint64_t v[4];

void sip_round() {
    // parallel is slower (due to overhead)
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
    // the last cell of words is filled here: TODO shift plutôt que modulo
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

    for (int i = 0; i<mplen; i++) {
        mp[i] = (uint8_t) (m>>(i*8));
        // printf("mp[%u] = %u\n", i, (uint8_t) (m>>(i*8)));
    }

    // printf("uint64_t %p\n", sip_hash_2_4(kp, mp, mplen));
    // printf("uint32_t %p\n", (uint32_t) sip_hash_2_4(kp, mp, mplen));
    return (uint32_t) sip_hash_2_4(kp, mp, mplen);
}

uint32_t coll_search2(uint32_t k, uint32_t (*fun)(uint32_t, uint32_t)) {
    uint32_t max_expected_size = (uint32_t) -1;
    bool *bon_array = calloc(max_expected_size, sizeof(bool));
    uint32_t m = 0;
    uint32_t result = sip_hash_fix32(k, m);

    while (bon_array[result] == false) {
        bon_array[result] = true;
        // fprintf(stderr, "id_collision: %i\n", id_collision);
        m++;
        result = sip_hash_fix32(k, m);
    }

    printf("Collision found: %p ", result);

    free(bon_array);

    return m;
}

bool search_hashtable(bool** table, uint32_t value) {
    uint16_t hash1 = (uint16_t) (value >> 16);
    uint16_t hash2 = (uint16_t) ((value << 16) >> 16);

    // printf("value = %0x, (%0x, %0x)\n", value, hash1, hash2);

    if (table[hash1] == NULL) {
        // bool new_array[1<<16] = {false};
        bool *new_array = calloc(1<<16, sizeof(bool));
        table[hash1] = new_array;
        table[hash1][hash2] = true;

        return false;
    } else {
        bool found = table[hash1][hash2];
        table[hash1][hash2] = true;

        return found;
    }
}

uint32_t coll_search(uint32_t k, uint32_t (*fun)(uint32_t, uint32_t)) {
    bool** hash_table = calloc(1<<16, sizeof(void *));
    uint32_t m = 0;
    uint32_t result = (*fun)(k, m);

    while (!search_hashtable(hash_table, result)) {
        m++;
        result = (*fun)(k, m);
        // printf("%u\n", m);
    }

    // free everything
    for (int i=0; i<1<<16; i++) {
        if (hash_table[i]) {
            free(hash_table[i]);
        }
    }
    free(hash_table);

    return m;
}

void main(int argc, char **argv) {

    if (argc != 2) {
        printf("Usage: %s {1, 4, 42, 5}\n 1 -> question 1: run sip_hash_2_4 on given examples\n 4 -> question 4: run coll_search\n 42 -> question 4: but with only one big array to be \n\tas fast as possible (requires a lot of memory)\n 5 -> question 5: statistics\n", argv[0]);
        exit(1);
    }

    srand(time(NULL));

    if (atoi(argv[1]) == 1) {

        uint64_t k[2];
        uint64_t k2[2];
        k[0] = 0x0706050403020100;
        k[1] = 0x0f0e0d0c0b0a0908;
        k2[0] = 0x0;
        k2[1] = 0x0;

        uint8_t m[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e};
        uint8_t m2[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};

        printf("siphash_2_4(k, m2, 8)    == %p\n", sip_hash_2_4(k, m2, 8));
        printf("siphash_2_4(k, NULL, 0)  == %p\n", sip_hash_2_4(k, NULL, 0));
        printf("siphash_2_4(k2, NULL, 0) == %p\n", sip_hash_2_4(k2, NULL, 0));

    } else if (atoi(argv[1]) == 4) {

        uint32_t key = rand();
        uint32_t nb_iterations;
        // double starttime, endtime;

        printf("Starting search of collision with a random key = %p...\n", key);

        // starttime = clock();
        nb_iterations = coll_search(key, sip_hash_fix32);
        // endtime = clock();

        printf("after %i tries!\n", nb_iterations);
        // printf("after %i iterations! (done in %fs.)\n", nb_iterations, (endtime-starttime));

    } else if (atoi(argv[1]) == 42) {

        uint32_t key = rand();
        uint32_t nb_iterations;
        // double starttime, endtime;

        printf("Starting search of collision with a random key = %p...\n", key);

        // starttime = clock();
        nb_iterations = coll_search2(key, sip_hash_fix32);
        // endtime = clock();

        printf("after %i tries!\n", nb_iterations);
        // printf("after %i iterations! (done in %fs.)\n", nb_iterations, (endtime-starttime));

    } else if (atoi(argv[1]) == 5) {

        uint32_t minimum = (uint32_t) -1;
        uint32_t maximum = 0;
        uint32_t sum = 0;
        uint32_t nb_iterations;
        uint32_t key = rand();

        for (int i=0; i<1000; i++) {
            key = rand();
            nb_iterations = 0;

            nb_iterations = coll_search(key, sip_hash_fix32);
            sum += nb_iterations;

            if (minimum > nb_iterations) {
                minimum = nb_iterations;
            }
            if (maximum < nb_iterations) {
                maximum = nb_iterations;
            }
        }

        printf("Done.\n Min = %u, Max = %u, Avg = %u\n", minimum, maximum, sum/1000);
        printf("2^16 = %u\n", 1<<16);
    }

    exit(0);
}
