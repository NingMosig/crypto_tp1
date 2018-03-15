#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

// constant
const uint64_t init[4] = {0x736f6d6570736575, 0x646f72616e646f6d, 0x6c7967656e657261, 0x7465646279746573};
const int d=4;
const int c=2;
// v is global for the readability of the code
uint64_t v[4];
// args
uint64_t k[2];
// uint8_t m[] = {0x0, 0x1, 0x3, 0x4, 0x5, 0x6, 0x7};
//? 0xff is uint8_t
uint8_t m[] = {0x00, 0x01, 0x03, 0x04, 0x05, 0x06, 0x07};
unsigned mlen;


void sipround() {
    // para
    v[0] += v[1];
    v[1] <<= 13;
    v[1] ^= v[0];
    v[0] <<= 32;
    // para
    v[2] += v[3];
    v[3] <<= 16;
    v[3] ^= v[2];
    // para
    v[2] += v[1];
    v[1] <<= 17;
    v[1] ^= v[2];
    v[2] <<= 32;
    // para
    v[0] += v[3];
    v[3] <<= 21;
    v[3] ^= v[0];
}


uint64_t siphash_2_4(uint64_t k[2], uint8_t *m, unsigned mlen) {
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
        words[i] = 0; // needed ? memset instead ?
        for (j = 0; j<8; j++) { // each word is 8 bytes long
            words[i] += (m[8*i+j] << 8*(j-1));
        }
        v[3] = v[3] ^ words[i];
    }
    // what if m[w-1] is empty ?
    // what if only part of m is useful ?
    // we need to stop when m stops 00 0f ff ff | ff ff ff ff | ff ff ff ff |...
    //                                  ^ stop here
    //                              ^ last word  ^ word        ^ word
    words[w-1] = 0;
    for (j = 0; j<(mlen%8); j++) { // stop when m ends if mlen is a multiple of 8 then nothing will be added here (modulo) the case j<7 is not a problem because here the last pack of m should not completely fill the cell of words
        words[w-1] += (m[8*(w-1)+j] << 8*(j-1));
    }
    // the last cell of words is filled here:
    words[w-1] += (mlen%256 << 56);
    // printf("%i\n", words[w-1]);
    v[3] = v[3] ^ words[w-1];

    // c siprounds
    for (i = 0; i<c; i++) {
        sipround();
    }
    for (i = 0; i<w; i++) {
        v[0] = v[0] ^ words[i];
    }

    // Finalization
    v[2] = v[2] ^ 0xff;
    // c siprounds
    for (i = 0; i<d; i++) {
        sipround();
    }

    result = v[0] ^ v[1] ^ v [2] ^ v[3];

    return result;
}

void main(int *argc, char **argv) {
    mlen = 8;
    k[0] = 0x0706050403020100;
    k[1] = 0x0f0e0d0c0b0a0908;
    // k[0] = 0;
    // k[1] = 0;

    printf("siphash_2_4(k, m, mlen) = %p\n", siphash_2_4(k, m, mlen));

    return;
}
