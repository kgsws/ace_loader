#include <libtransistor/nx.h>
#include <libtransistor/ipc.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "defs.h"
#include "nro.h"
#include "sha256.h"
#include "memory.h"

void *nro_load(int nsize) // TODO: size checks on heap
{
	result_t r;
	SHA256_CTX ctx;
	int size = *(uint32_t*)(heap_base + 0x18);
	uint32_t *nrru32 = (uint32_t*)(heap_base + nsize);
	void *base;

	printf("- NRO %iB (%iB) bss %iB\n", size, nsize, *(uint32_t*)(heap_base + 0x38));

	printf("- prepare NRR\n");
	// prepare NRR
	nrru32[0] = 0x3052524E; // NRR0
	nrru32[(0x338 >> 2) + 0] = 0x1000; // size
	nrru32[(0x340 >> 2) + 0] = 0x350; // hash offset
	nrru32[(0x340 >> 2) + 1] = 0x1; // hash count

	printf("- HASH NRO\n");
	// get hash
	sha256_init(&ctx);
	sha256_update(&ctx, (uint8_t*)heap_base, nsize);
	sha256_final(&ctx, (uint8_t*)&nrru32[0xD4]); // hash

	printf("- init ldr:ro\n");
	// initialize RO
	r = ro_init();
	if(r)
	{
		printf("- ldr:ro initialization error 0x%06X\n", r);
		return NULL;
	}

	printf("- load NRR\n");
	// load NRR
	r = ro_load_nrr(nrru32, nrru32[(0x338 >> 2) + 0]);
	if(r)
	{
		printf("- NRR load error 0x%06X\n", r);
		return NULL;
	}

	printf("- load NRO\n");
	// load NRO
	r = ro_load_nro(&base, heap_base, size, heap_base + size, *(uint32_t*)(heap_base + 0x38));
	if(r)
	{
		printf("- NRO load error 0x%06X\n", r);
		return NULL;
	}

/*
	printf("- wait\n");
	// debug
	svcSleepThread(2000*1000*1000);

	printf("- unload NRR\n");
	// debug: unload NRR
	r = ro_unload_nrr(nrru32, nrru32[(0x338 >> 2) + 0]);
	if(r)
	{
		printf("- NRR unload error 0x%06X\n", r);
		return NULL;
	}
*/
	printf("- close ld:ro\n");
	// close RO
	ro_finalize();

	printf("- all passed\n");

	return base;
}

