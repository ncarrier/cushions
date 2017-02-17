/*
 * most of the code is taken from:
 *    https://github.com/ncarrier/libpimp
 */
#define _GNU_SOURCE
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include <unistd.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdbool.h>

#define CH_LOG_TAG sock_handler
#include <cushions_handler.h>

struct sock_cushions_handler {
	struct ch_handler handler;
};

#define UNIX_PATH_MAX 108
#ifndef UNIX_ADDRSTRLEN
#define UNIX_ADDRSTRLEN 108
#endif /* UNIX_ADDRSTRLEN */

enum addr_type {
	ADDR_INET	= AF_INET,
	ADDR_INET6	= AF_INET6,
	ADDR_UNIX	= AF_UNIX,
};

struct addr {
	enum addr_type type;
	union {
		struct sockaddr addr;
		struct sockaddr_in in;
		struct sockaddr_in6 in6;
		struct sockaddr_un un;
	};
	socklen_t len;
};

static int unix_addr_from_str(struct addr *addr, const char *str)
{
	addr->type = ADDR_UNIX;
	addr->un.sun_family = AF_UNIX;
	snprintf(addr->un.sun_path, UNIX_PATH_MAX, "%s", str);
	if (addr->un.sun_path[0] == '@') /* address is abstract */
		addr->un.sun_path[0] = 0;

	return 0;
}

static int inet_addr_from_str(struct addr *addr, const char *str)
{
	int ret;
	char *endptr;
	char *port;
	char *ipstr;
	unsigned long ulport;
	size_t len;

	addr->type = ADDR_INET;
	addr->in.sin_family = AF_INET;
	len = strlen(str) + 1;
	ipstr = alloca(len);
	snprintf(ipstr, len, "%s", str);
	port = strchr(ipstr, ':');
	if (port == NULL)
		return -EINVAL;
	*(port++) = '\0';
	ulport = strtoul(port, &endptr, 0);
	if (*endptr != '\0' || ulport > 0xffff)
		return -EINVAL;

	ret = inet_pton(AF_INET, ipstr, &addr->in.sin_addr);
	if (ret <= 0)
		return -EINVAL;
	addr->in.sin_port = htons(ulport);

	return 0;
}

static int inet6_addr_from_str(struct addr *addr, const char *str)
{
	int ret;
	char *endptr;
	char *port;
	char *ipstr;
	unsigned long ulport;
	size_t len;

	addr->type = ADDR_INET6;
	addr->in6.sin6_family = AF_INET6;
	len = strlen(str) + 1;
	ipstr = alloca(len);
	snprintf(ipstr, len, "%s", str);
	port = strrchr(ipstr, ':');
	if (port == NULL)
		return -EINVAL;
	*(port++) = '\0';
	ulport = strtoul(port, &endptr, 0);
	if (*endptr != '\0' || ulport > 0xffff)
		return -EINVAL;

	ret = inet_pton(AF_INET6, ipstr, &addr->in6.sin6_addr);
	if (ret <= 0)
		return -EINVAL;
	addr->in6.sin6_port = htons(ulport);

	return 0;
}

static int addr_from_str(struct addr *addr, const char *str)
{
	if (addr == NULL)
		return -EINVAL;
	memset(addr, 0, sizeof(*addr));

	if (str == NULL || *str == '\0')
		return -EINVAL;

	if (strncmp(str, "unix:", 5) == 0)
		return unix_addr_from_str(addr, str + 5);
	if (strncmp(str, "inet:", 5) == 0)
		return inet_addr_from_str(addr, str + 5);
	if (strncmp(str, "inet6:", 5) == 0)
		return inet6_addr_from_str(addr, str + 6);

	return -EAFNOSUPPORT;
}

static socklen_t addr_len(struct addr *addr)
{
	if (addr == NULL)
		return 0;

	if (addr->len == 0) {
		switch (addr->type) {
		case ADDR_INET:
			addr->len = sizeof(addr->in);
			break;

		case ADDR_INET6:
			addr->len = sizeof(addr->in6);
			break;

		case ADDR_UNIX:
			addr->len = sizeof(addr->un);
			break;
		}
	}

	return addr->len;
}


static int pimp_server(const char *address, int flags, int backlog)
{
	int ret;
	int old_errno;
	int server;
	struct addr addr;
	socklen_t len;

	ret = addr_from_str(&addr, address);
	if (ret != 0) {
		errno = -ret;
		return -1;
	}

	server = socket(addr.addr.sa_family, SOCK_STREAM | flags, 0);
	if (server == -1)
		return -1;
	ret = setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 },
			sizeof(int));
	if (ret == -1)
		return -1;

	len = addr_len(&addr);
	ret = bind(server, &addr.addr, len);
	if (ret == -1) {
		old_errno = errno;
		close(server);
		errno = old_errno;
		return -1;
	}

	ret = listen(server, backlog);
	if (ret == -1) {
		old_errno = errno;
		close(server);
		errno = old_errno;
		return -1;
	}

	return server;
}

static FILE *pimp_accept(int server, int flags)
{
	int ret;
	int fdc;
	int old_errno;
	FILE *client;
	struct addr addr;

	if (server < 0) {
		errno = EINVAL;
		return NULL;
	}

	addr.len = sizeof(addr.un);
	fdc = accept4(server, &addr.addr, &addr.len, flags);
	if (fdc == -1)
		return NULL;
	addr.type = addr.addr.sa_family;

	client = fdopen(fdc, "r+");
	if (client == NULL) {
		old_errno = errno;
		close(fdc);
		errno = old_errno;
		return NULL;
	}
	ret = setvbuf(client, NULL, _IONBF, 0);
	if (ret != 0) {
		old_errno = errno;
		fclose(client);
		errno = old_errno;
		return NULL;
	}

	return client;
}

static FILE *pimp_client(const char *address, int flags)
{
	int ret;
	int fd;
	int old_errno;
	FILE *client;
	struct addr addr;

	ret = addr_from_str(&addr, address);
	if (ret != 0) {
		errno = -ret;
		return NULL;
	}

	fd = socket(addr.addr.sa_family, SOCK_STREAM | flags, 0);
	if (fd == -1)
		return NULL;

	do {
		ret = connect(fd, &addr.addr, addr_len(&addr));
		if (ret == -1) {
			if (errno == ECONNREFUSED) {
				LOGD("server not responding, retry in 1 sec");
				sleep(1);
				continue;
			}
			old_errno = errno;
			perror("connect");
			close(fd);
			errno = old_errno;
			return NULL;
		}
	} while (ret != 0);

	client = fdopen(fd, "r+");
	if (client == NULL) {
		old_errno = errno;
		close(fd);
		errno = old_errno;
		return NULL;
	}
	ret = setvbuf(client, NULL, _IONBF, 0);
	if (ret != 0) {
		old_errno = errno;
		fclose(client);
		errno = old_errno;
		return NULL;
	}

	return client;
}

static FILE *server_socket(struct ch_handler *handler, const char *path,
		const struct ch_mode *mode)
{
	int __attribute__((cleanup(ch_fd_cleanup)))server = -1;

	server = pimp_server(path, 0, 1);
	if (server == -1)
		return NULL;

	return pimp_accept(server, 0);
}

static FILE *client_socket(struct ch_handler *handler, const char *path,
		const struct ch_mode *mode)
{
	return pimp_client(path, 0);
}

static FILE *sock_cushions_fopen(struct ch_handler *handler,
		const char *path, const char *full_path, const char *scheme,
		const struct ch_mode *mode)
{
	LOGD(__func__);

	if (*scheme == 's')
		return server_socket(handler, path, mode);
	else
		return client_socket(handler, path, mode);
}

static bool sock_handler_handles(struct ch_handler *handler,
		const char *s)
{
	return (*s == 's' || *s == 'c') && strcmp(s + 1, "sock") == 0;
}

static struct sock_cushions_handler sock_cushions_handler = {
	.handler = {
		.name = "sock",
		.fopen = sock_cushions_fopen,
		.handles = sock_handler_handles,
	},
};

static __attribute__((constructor)) void sock_cushions_handler_constructor(
		void)
{
	int ret;

	LOGI(__func__);

	ret = ch_handler_register(&sock_cushions_handler.handler);
	if (ret < 0)
		LOGW("cushions_handler_register(sock_cushions_handler): %s",
				strerror(-ret));
}
