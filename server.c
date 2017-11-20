#include <libtransistor/nx.h>
#include <libtransistor/ipc.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "defs.h"
#include "server.h"
#include "memory.h"

static int sockets[2];

int server_init()
{
	int sck;
	struct sockaddr_in server_addr =
	{
		.sin_family = AF_INET,
		.sin_port = htons(2991)
	};

	sck = bsd_socket(2, 1, 6); // AF_INET, SOCK_STREAM, PROTO_TCP
	if(sck < 0)
		return 1;

	if(bsd_bind(sck, (struct sockaddr *)&server_addr, sizeof(server_addr)))
	{
		bsd_close(sck);
		return 2;
	}

	if(bsd_listen(sck, 1) < 0)
	{
		bsd_close(sck);
		return 3;
	}

	sockets[0] = sck;
	sockets[1] = -1;
	return 0;
}

void server_loop()
{
	struct sockaddr_in client_addr;
	int ret;
	uint32_t idx;
	void *ptr = NULL;
	uint64_t size = 0;

	printf("- starting push server ...\n");
	while(1)
	{
		if(sockets[1] == -1)
		{
			// server
			idx = sizeof(client_addr);
			ret = bsd_accept(sockets[0], (struct sockaddr *)&client_addr, &idx);
			if(ret < 0)
			{
				ret = bsd_errno;
				break;
			}
			sockets[1] = ret;
			printf("- client %i.%i.%i.%i:%i connected\n", client_addr.sin_addr.s_addr & 0xFF, (client_addr.sin_addr.s_addr >> 8) & 0xFF, (client_addr.sin_addr.s_addr >> 16) & 0xFF, client_addr.sin_addr.s_addr >> 24, ntohs(client_addr.sin_port));
			// reset pointer
			ptr = heap_base;
			size = heap_size;
		} else
		{
			// client
			ret = bsd_recv(sockets[1], ptr, RECV_BLOCKSIZE, 0);
			if(ret < 0)
			{
				bsd_close(sockets[1]);
				sockets[1] = -1;
				printf("- client dropped\n");
				continue;
			}
			if(ret == 0)
			{
				bsd_close(sockets[1]);
				sockets[1] = -1;
				printf("- client disconnected\n");
				// got all the data
				if(*(uint32_t*)heap_base == 0x74697865) // 'exit'
				{
					printf("- exit loader\n");
					return;
				}
				*(char*)ptr = 0;
				printf("- got %luB from client:\n%s", ptr - heap_base, heap_base);
				continue;
			}
			// move pinter
			size -= ret;
			ptr += ret;
			// memory check
			if(size < RECV_BLOCKSIZE)
			{
				bsd_close(sockets[1]);
				sockets[1] = -1;
				printf("- out of memory; client dropped\n");
				continue;
			}
		}
	}
	printf("- server error 0x%06X\n", ret);
}

