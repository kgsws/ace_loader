
#define LOAER_CTX_ID	0x007874635f656361
#define LOADER_VERSION	0

// return flags
#define RETF_KEEP_LOADED	1	// do not unload NRO from memory
#define RETF_RUN_MEMINFO	2	// show memory map after cleanup

// loader context
typedef struct
{
	uint64_t id; // LOAER_CTX_ID
	uint64_t version; // can be sequential or pseudorandom
	void *mem_base; // usable contiguous block of free memory
	uint64_t mem_size; // ^ size
	int std_socket; // socket that loader uses for stdout
	int ro_handle; // ldr:ro service handle
	uint64_t return_flags;
} nro_loader_ctx_t;

result_t nro_load(int in_size);
uint64_t nro_start();

uint64_t nro_execute(int in_size);

