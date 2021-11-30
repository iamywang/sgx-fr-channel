#include "Enclave_t.h"

char secrets[21] = "secrets_in_enclaves.";
int junk = 0;

void ecall_leak_byte(char *shared_array, int i) {
    junk &= shared_array[secrets[i] * 4096];
}
