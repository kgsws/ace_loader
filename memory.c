#include <libtransistor/nx.h>
#include <libtransistor/ipc.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "defs.h"
#include "memory.h"

void *wkBase;

// map area
void *map_base;
static heap_map_t heap_map[HEAP_MAP_SIZE];
static int heap_map_cur;

// dump address space mapping
void mem_info()
{
	void *addr = NULL;
	memory_info_t minfo;
	uint32_t pinfo;

	while(1)
	{
		if(svcQueryMemory(&minfo, &pinfo, addr))
		{
			printf("- querry fail\n");
			return;
		}

		printf("mem 0x%016lX size 0x%016lX; %i %i [%i]\n", (uint64_t)minfo.base_addr, minfo.size, minfo.permission, minfo.memory_type, minfo.memory_attribute);

		addr = minfo.base_addr + minfo.size;
		if(!addr)
			break;
	}
}

// create one huge contiguous block from HEAP
// fill 'heap_map' for unmapping
int mem_make_block()
{
	void *addr = NULL;
	memory_info_t minfo;
	uint32_t pinfo;
	void *map = map_base;

	heap_map_cur = 0;

	while(1)
	{
		if(svcQueryMemory(&minfo, &pinfo, addr))
			return 1;

		if(minfo.permission == 3 && minfo.memory_type == 5 && minfo.memory_attribute == 0)
		{
			int ret = svcMapMemory(map, minfo.base_addr, minfo.size);
			if(!ret)
			{
				heap_map[heap_map_cur].src = minfo.base_addr;
				heap_map[heap_map_cur].dst = map;
				heap_map[heap_map_cur].size = minfo.size;
				map += minfo.size;
				if(heap_map_cur == HEAP_MAP_SIZE)
				{
					printf("- out of map storage space\n");
					break;
				}
			} else
				printf("- svcMapMemory failed 0x%06X\n", ret);
		}

		addr = minfo.base_addr + minfo.size;
		if(!addr)
			break;
	}

	printf("- mapped %luB contiguous memory block\n", map - map_base);

	return 0;
}

// remove 'uncached' flag from all HEAP pages
int mem_heap_cleanup()
{
	void *addr = NULL;
	memory_info_t minfo;
	uint32_t pinfo;

	while(1)
	{
		if(svcQueryMemory(&minfo, &pinfo, addr))
			return 1;

		if(minfo.permission == 3 && minfo.memory_type == 5 && minfo.memory_attribute & 8)
			printf("- svcSetMemoryAttribute 0x%06X\n", svcSetMemoryAttribute(minfo.base_addr, minfo.size, 0, 0));

		addr = minfo.base_addr + minfo.size;
		if(!addr)
			break;
	}

	return 0;
}

int mem_install_hook(void *hook_func)
{
	void *addr = NULL;
	memory_info_t minfo;
	uint32_t pinfo;
	uint64_t *ptr = NULL;

	while(1)
	{
		if(svcQueryMemory(&minfo, &pinfo, addr))
			return 1;

		if(minfo.size == WK_SIZE && minfo.permission == 5 && minfo.memory_type == 3)
			wkBase = minfo.base_addr;
		if(minfo.size == WK_MEM_SIZE && minfo.permission == 3 && minfo.memory_type == 4)
		{
			ptr = minfo.base_addr + 0x62E0;
			*ptr = (uint64_t)hook_func; // original call 0x16e6b4 from 0x12b524
		}
		addr = minfo.base_addr + minfo.size;
		if(!addr)
			break;
	}

	if(!ptr || !wkBase)
		return 1;
	return 0;
}

