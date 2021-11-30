#include "Enclave_u.h"
#include <errno.h>

typedef struct ms_ecall_leak_byte_t {
	char* ms_shared_array;
	int ms_i;
} ms_ecall_leak_byte_t;

static const struct {
	size_t nr_ocall;
	void * table[1];
} ocall_table_Enclave = {
	0,
	{ NULL },
};
sgx_status_t ecall_leak_byte(sgx_enclave_id_t eid, char* shared_array, int i)
{
	sgx_status_t status;
	ms_ecall_leak_byte_t ms;
	ms.ms_shared_array = shared_array;
	ms.ms_i = i;
	status = sgx_ecall(eid, 0, &ocall_table_Enclave, &ms);
	return status;
}

