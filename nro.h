
#define NRO_MAX_ARG_BUF	0x1000
#define NRO_MAX_ARGS	16

#define NRO_MAGIC	0x304f524e

result_t nro_load(void *load_base, int in_size);
uint64_t nro_start();

uint64_t nro_execute(void *load_base, int in_size);

void nro_arg_name(const char *text);
void nro_add_arg(const char *text);
