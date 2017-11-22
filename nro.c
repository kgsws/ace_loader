#include <libtransistor/nx.h>
#include <libtransistor/ipc.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "defs.h"
#include "nro.h"
#include "sha256.h"
#include "memory.h"

#define NRR_SIZE	0x1000

static libtransistor_context_t loader_context;
static void *nro_base;

uint64_t nro_start()
{
	uint64_t (*entry)(libtransistor_context_t*) = nro_base + 0x80;
	uint64_t ret;

	// generate memory block
	if(mem_make_block())
	{
		printf("- failed to map memory block\n");
		return -1;
	}

	// always refill context
	loader_context.version = LIBTRANSISTOR_CONTEXT_VERSION;
	loader_context.size = sizeof(loader_context);
	loader_context.log_buffer = NULL; // out
	loader_context.log_length = 0; // out
	loader_context.argv = NULL;
	loader_context.argc = 0;
	loader_context.mem_base = map_base;
	loader_context.mem_size = map_size;
	loader_context.std_socket = std_sck;
	//loader_context.ro_handle = ro_object.object_id; // TODO: check if correct and make ro_object accessible
	loader_context.return_flags = 0; // out
	
	// release sm
	sm_finalize();

	// run NRO
	ret = entry(&loader_context);

	if(loader_context.log_buffer != NULL && *loader_context.log_length > 0) {
		loader_context.log_buffer[*loader_context.log_length] = 0;
		printf("LOG:\n%s", loader_context.log_buffer);
	}
	
	// release memory block
	mem_destroy_block(); // TODO: panic on fail?
	
	// get sm again
	sm_init(); // TODO: panic on fail?

	return ret;
}

result_t nro_load(int in_size)
{
	result_t r;
	SHA256_CTX ctx;
	int nro_size = *(uint32_t*)(heap_base + 0x18);
	uint32_t bss_size = *(uint32_t*)(heap_base + 0x38);
	uint32_t nro_id = *(uint32_t*)(heap_base + 0x10);
	uint32_t *nrru32 = (uint32_t*)(heap_base + nro_size + bss_size);

	if(in_size < 0x1000 || nro_id != 0x304f524e || nro_size != in_size || nro_size & 0xFFF)
	{
		printf("- NRO is invalid\n");
		return 1;
	}

	if(nro_size + bss_size + NRR_SIZE > heap_size)
	{
		printf("- NRO (+bss) is too big\n");
		return 2;
	}

	printf("- NRO %iB (%iB) bss %iB\n", nro_size, in_size, bss_size);

	// prepare NRR
	nrru32[0] = 0x3052524E; // NRR0
	nrru32[(0x338 >> 2) + 0] = NRR_SIZE; // size
	nrru32[(0x340 >> 2) + 0] = 0x350; // hash offset
	nrru32[(0x340 >> 2) + 1] = 0x1; // hash count

	// get hash
	sha256_init(&ctx);
	sha256_update(&ctx, (uint8_t*)heap_base, nro_size);
	sha256_final(&ctx, (uint8_t*)&nrru32[0xD4]); // hash

	// load NRR
	r = ro_load_nrr(nrru32, NRR_SIZE);
	if(r)
	{
		printf("- NRR load error 0x%06X\n", r);
		return r;
	}

	// load NRO
	r = ro_load_nro(&nro_base, heap_base, nro_size, heap_base + nro_size, bss_size);
	if(r)
	{
		printf("- NRO load error 0x%06X\n", r);
		return r;
	}

	printf("- NRO base at 0x%016lX\n", (uint64_t)nro_base);

	// unload NRR
	r = ro_unload_nrr(nrru32);
	if(r)
	{
		printf("- NRR unload error 0x%06X\n", r);
		return r;
	}

	return 0;
}

result_t nro_unload()
{
	result_t r = 0;

	// unload by default, do not if requested
	if(loader_context.return_flags & RETF_KEEP_LOADED)
		printf("- NRO left loaded\n");
	else
	{
		// unload NRO
		r = ro_unload_nro(nro_base, heap_base);
		if(r)
			printf("- NRO unload error 0x%06X\n", r);
	}

	// show memory if requested
	if(loader_context.return_flags & RETF_RUN_MEMINFO)
		mem_info();

	return r;
}

uint64_t nro_execute(int in_size)
{
	uint64_t ret;
	result_t r;

	r = nro_load(in_size);
	if(r)
		return r;

	ret = nro_start();

	nro_unload(); // TODO: panic on fail?

	return ret;
}

