#include <stdio.h>
#include <stdint.h>


uint64_t init[4] = {0x736f6d6570736575, 0x646f72616e646f6d, 0x6c7967656e657261, 0x7465646279746573};

uint64_t sipround(uint64_t v[4]) {
    uint64_t result;
    // TODO
    return result;
}


uint64_t siphash_2_4(uint64_t k[2], uint8_t *m, unsigned mlen) {
    uint64_t result;
    uint8_t w = (mlen + 1)%8 ? (mlen + 1)/8 + 1 : (mlen + 1) / 8;
    uint64_t v[4] = malloc(4*sizeof(uint64_t));
    uint64_t words[w] = calloc(w, sizeof(uint64_t));

    // initialization
    for (int i = 0; i<4; i++) {
        v[i] = k[i%2] ^ init[4];
    }

    // compression
    for (int i = 0; i<w-1; i++) {
        words[i] = *(m + 8*i);
        v[3] = v[3] ^ words[i];
    }
    words[w-1] = *(m + 8*w);
    words[w-1] += mlen%256 << 62;
    v[3] = v[3] ^ words[w-1];

    // c siprounds
    for (int i = 0; i<c; i++) {
        sipround();
    }
    for (int i = 0; i<w; i++) {
        v[0] = v[0] ^ words[i];
    }

    // Finalization
    v[2] = v[2] ^ 0xff;
    // c siprounds
    for (int i = 0; i<d; i++) {
        sipround();
    }

    result = v[0] ^ v[1] ^ v [2] ^ v[3];

    return result;
}

int main() {
    siphash_2_4();
}
