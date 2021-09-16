/* See LICENSE file for copyright and license details. */
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>


#ifndef CAPACITY
# define CAPACITY (1 << 20)
#endif


int
main(int argc, const char *argv[])
{
	static char buf[1 << 16];
	int rw1[2], rw2[2], n = 0;
	ssize_t r, p, w;
	size_t sum = 0, totsum = 0;
	struct timespec start, end;
	double rate;
	int capacity, flags;

	if (!argc)
		*argv = "limit";

	dprintf(STDERR_FILENO, "Requested capacity: %i\n", CAPACITY);
	if (pipe(rw1) || pipe(rw2))
		goto fail;
	capacity = fcntl(*rw1, F_SETPIPE_SZ, CAPACITY);
	if (capacity < 0 || fcntl(*rw2, F_SETPIPE_SZ, capacity) != capacity)
		goto fail;
	dprintf(STDERR_FILENO, "Configured capacity: %i\n", capacity);

	flags = fcntl(rw1[1], F_GETFL);
	if (flags < 0 || fcntl(rw1[1], F_SETFL, flags |= O_NONBLOCK) < 0)
		goto fail;
	for (n = capacity, capacity = 0; n; n -= (int)r, capacity += (int)r) {
		if ((r = write(rw1[1], buf, (size_t)n)) < 0) {
			if (errno != EAGAIN)
				goto fail;
			n -= 1;
			r = 0;
		}
	}
	dprintf(STDERR_FILENO, "Supported capacity: %i\n", capacity);
	if (fcntl(rw1[1], F_SETFL, flags ^= O_NONBLOCK) < 0)
		goto fail;

	if (clock_gettime(CLOCK_MONOTONIC, &start))
		goto fail;
	for (;;) {
		r = splice(rw1[0], NULL, rw2[1], NULL, (size_t)capacity, 0);
		if (r <= 0) {
			if (r < 0)
				goto fail;
			goto done;
		}
		for (p = 0; p < r; p += w) {
			w = splice(rw2[0], NULL, rw1[1], NULL, (size_t)(r - p), 0);
			if (w <= 0) {
				if (w < 0)
					goto fail;
				goto done;
			}
		}
		sum += (size_t)r << 1;
		if (++n == 100000) {
			if (clock_gettime(CLOCK_MONOTONIC, &end))
				goto fail;
			n = 0;
			start.tv_sec  = end.tv_sec  - start.tv_sec;
			start.tv_nsec = end.tv_nsec - start.tv_nsec;
			rate = (double)start.tv_nsec;
			rate /= (double)1000000000.f;
			rate += (double)start.tv_sec;
			rate = (double)sum / rate;
			rate /= (double)1000000000.f;
			totsum += sum;
			sum = 0;
			dprintf(STDERR_FILENO, "  %lf GB/s\033[K\n\033[A", rate);
			start = end;
		}
	}

done:
	return 0;

fail:
	perror(*argv);
	return 1;
}
