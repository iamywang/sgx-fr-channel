# SGX-FR-Channel

## How to run

First you should install Intel SGX and start the service `aesmd.service`, then following these commands:

```shell
$ git clone https://github.com/iamywang/sgx-fr-channel.git
$ make
$ ./app
```

Expected output of this program:

```shell
$ ./app
THRESHOLD: 169, AVG_HIT: 64.13, AVG_MISS: 380.31.
reading secret at 0x7ffeed2dd93f.. best: 0x73(s), score: 100/100.
reading secret at 0x7ffeed2dd940.. best: 0x65(e), score: 100/100.
...
reading secret at 0x7ffeed2dd951.. best: 0x73(s), score: 100/100.
reading secret at 0x7ffeed2dd952.. best: 0x2e(.), score: 100/100.
```

## Flush-Reload Covert Channel

### Example code

Sender:

```C
void ecall_leak_byte(char *shared_array, int i) {
    junk &= shared_array[secrets[i] * 4096];
}
```

Receiver:

```C
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
```

### Steps

The sender: defined in trusted code `Enclave.cpp`.

The receiver: defined in trusted code `App.cpp`.

- R: Flush the shared array `shared` with `_mm_clflush(&shared[i * 4096]);`.
- R: Trigger the sender to encode the secret.
- S: Encode the secret through shared array `junk &= shared_array[secrets[i] * 4096];`.
- R: Reload the shared array to decode data.
