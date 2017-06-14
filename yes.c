/* See LICENSE file for copyright and license details. */

#define _GNU_SOURCE
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <alloca.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#ifndef CAPACITY
# define CAPACITY (1 << 20)
#endif


static char *argv0;
static void (*writeall)(char *, size_t);


static void
writeall_write(char *buf, size_t n)
{
	ssize_t r;
	for (; n; n -= (size_t)r, buf += r)
		if ((r = write(STDOUT_FILENO, buf, n)) < 0)
			perror(argv0), exit(1);
}

#if 0
static void
writeall_vmsplice(char *buf, size_t n)
{
	struct iovec iov;
	ssize_t r;
	iov.iov_base = buf;
	iov.iov_len = n;
	while (iov.iov_len) {
		r = vmsplice(STDOUT_FILENO, &iov, 1, 0);
		if (r < 0) {
			if (errno == EINVAL) {
				writeall = writeall_write;
				writeall_write(iov.iov_base, iov.iov_len);
				return;
			}
			perror(argv0), exit(1);
		}
		iov.iov_base += r;
		iov.iov_len -= (size_t)r;
	}
}
#endif

int
main(int argc, char *argv[])
{
	size_t n, len, m = 0, p, cap = 0;
	char *buf, *b;
	int i, sz, fds[2], flags;
	ssize_t r;

	if (!argc)
		argc = 1;
	argv0 = argv[0] ? argv[0] : "yes";
	if (argc == 1) {
		argc = 2;
		argv = (char *[]){argv0, "y"};
	}
	n = (size_t)(argc - 1);

	writeall = writeall_write;

	for (i = 1; i < argc; i++)
		n += strlen(argv[i]);
	b = buf = alloca(n);
	for (i = 1; i < argc; i++) {
		b = stpcpy(b, argv[i]);
		*b++ = ' ';
	}
	b[-1] = '\n';

	if (pipe(fds))
		goto fail;

	if (fds[0] == STDOUT_FILENO || fds[1] == STDOUT_FILENO) {
		errno = EBADF;
		goto fail;
	}

	if ((flags = fcntl(fds[1], F_GETFL)) == -1)
		goto fail;
	if (fcntl(fds[1], F_SETFL, flags | O_NONBLOCK) == -1)
		goto fail;

	if ((sz = fcntl(STDOUT_FILENO, F_GETPIPE_SZ)) > 0) {
		if (sz < CAPACITY)
			if (fcntl(STDOUT_FILENO, F_SETPIPE_SZ, CAPACITY) != -1)
				sz = CAPACITY;
		fcntl(fds[1], F_SETPIPE_SZ, sz);
	} else {
		fcntl(fds[1], F_SETPIPE_SZ, 3 << 16);
	}

	for (b = buf, p = n;;) {
		r = write(fds[1], b, p);
		if (r < 0) {
			if (errno == EAGAIN)
				break;
			goto fail;
		}
		b += r;
		cap += (size_t)r;
		p -= (size_t)r;
		if (!p) {
			b = buf;
			p = n;
			m += 1;
		}
	}

	cap -= cap % n;
	len = n;
	n *= m;
	if (m)
		cap = n;

	for (;;) {
		r = tee(fds[0], STDOUT_FILENO, cap, 0);
		if (r < (ssize_t)n) {
			if (r < 0) {
				if (errno == EINVAL)
					goto fallback;
				goto fail;
			}
			r %= len;
			writeall(buf + r, len - r);
		}
	}

fallback:
	for (p = 0;; p = (p + (size_t)r) % len) {
		if ((r = write(STDOUT_FILENO, buf + p, len - p)) < 0)
			goto fail;
	}

	return 0;

fail:
	perror(argv0);
	return 1;
}
