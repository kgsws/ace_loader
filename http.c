#include <libtransistor/nx.h>
#include <libtransistor/ipc.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "defs.h"
#include "http.h"

static FILE http_stdout;
static int sck; // HTTP socket

static const char http_get_template[] = "GET /files/%s HTTP/1.1\r\nHost: %s\r\nUser-Agent: ACE loader\r\nAccept-Encoding: none\r\nConnection: close\r\n\r\n";
static char http_host[] = HTTP_HOSTNAME;

static struct sockaddr_in server_addr =
{
	.sin_family = AF_INET,
	.sin_port = htons(80),
};

static int stdout_http(struct _reent *reent, void *v, const char *ptr, int len)
{
	bsd_send(sck, ptr, len, 0);
	return len;
}

static int parse_header(char *buf, int len, int *offset, int *got)
{
	char *ptr = buf;
	char *pptr = buf;
	int ret;
	int state = 0;
	int content_length = 0;

	while(len)
	{
		// get some bytes
		ret = bsd_recv(sck, ptr, len, 0);
		if(ret <= 0)
		{
			printf("- bsd_recv %i\n", ret);
			return -1;
		}
		ptr += ret;
		// parse line(s)
		while(1)
		{
			char *tptr = pptr;
			char *eptr = pptr + ret;
			// get line end
			while(tptr < eptr)
			{
				if(*tptr == '\n')
				{
					if(tptr - pptr <= 1)
					{
						// empty line, header ending
						if(state)
						{
							*offset = (tptr + 1) - buf;
							*got = (ptr - buf) - *offset;
							return content_length;
						} else
							return -2;
					}
					// got one line, parse it
					if(state)
					{
						if(!strncmp(pptr, "Content-Length:", 15))
							sscanf(pptr + 15, "%d", &content_length);
					} else
					{
						int v1, v2, res;
						// HTTP response
						state = 1;
						if(sscanf(pptr, "HTTP/%d.%d %d", &v1, &v2, &res) != 3 || !res)
							return -1;
						if(res != 200)
							return -res;
					}
					// go next
					pptr = tptr + 1;
					break;
				}
				tptr++;
			}
			if(tptr == pptr)
				// no more lines left
				break;
		}
		// go next
		len -= ret;
	}
	
	return -1;
}

int http_init()
{
//	int ret;
//	struct addrinfo *ai = NULL;
//	struct addrinfo hints;
//	struct sockaddr_in *sin;

//	hints.ai_family = AF_INET;
//	hints.ai_socktype = SOCK_STREAM;

	// get IP - fails in bsd.c
/*	ret = bsd_getaddrinfo(HTTP_HOSTNAME, "6767", &hints, &ai);
	if(ret)
	{
		printf("- failed to getaddrinfo: %d, 0x%x\n", ret, bsd_result);
		return ret;
	}
	if(!ai)
		return 1;
	if(ai->ai_family != AF_INET)
		return 2;
	sin = (struct sockaddr_in*)ai->ai_addr;
	server_addr.sin_addr.s_addr = sin->sin_addr.s_addr;
	bsd_freeaddrinfo(ai);

	printf("- host IP is %i.%i.%i.%i\n", server_addr.sin_addr.s_addr & 0xFF, (server_addr.sin_addr.s_addr >> 8) & 0xFF, (server_addr.sin_addr.s_addr >> 16) & 0xFF, server_addr.sin_addr.s_addr >> 24);
*/
	// for now, use hardcoded IP
	server_addr.sin_addr.s_addr = make_ip(192,168,1,47);

	// prepare HTTP stdout
	http_stdout._write = stdout_http;
	http_stdout._flags = __SWR | __SNBF;
	http_stdout._bf._base = (void*)1;

	return 0;
}

int http_get_nro(const char *path, void *buf, int size)
{
	char temp[1024];
	int ret;
	int offs, got;

	sck = bsd_socket(2, 1, 6); // AF_INET, SOCK_STREAM, PROTO_TCP
	if(sck < 0)
		return 1;

	if(bsd_connect(sck, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0)
	{
		bsd_close(sck);
		return 2;
	}

	// make a request
	stdout = &http_stdout;
	printf(http_get_template, path, http_host);
	stdout = &custom_stdout;

	// get an answer
	ret = parse_header(temp, sizeof(temp), &offs, &got);
	// TEST
	printf("- file size: %iB (offs %iB, got %iB)\n", ret, offs, got);
	if(ret > 0)
	{
		if(got)
		{
			if(size + offs < sizeof(temp))
				temp[size + offs] = 0;
			else
				temp[sizeof(temp)-1] = 0;
			printf("- contents:\n%s\n", (const char*)(temp + offs));
		} else
		{
			got = bsd_recv(sck, temp, sizeof(temp)-1, 0);
			temp[got] = 0;
			printf("- got contents:\n%s\n", (const char*)temp);
		}
	}

	bsd_close(sck);
	return 0;
}

