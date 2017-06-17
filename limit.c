/* See LICENSE file for copyright and license details. */

#define _GNU_SOURCE
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
main(int argc, char *argv[])
{
	static char buf[1 << 16];
	int rw1[2], rw2[2], n = 0, m;
	ssize_t r, p, w;
	size_t sum = 0, totsum = 0;
	struct timespec start, end;
	double rate;

	if (!argc)
		*argv = "limit";

	if (pipe(rw1) || fcntl(*rw1, F_SETPIPE_SZ, CAPACITY) != CAPACITY)
		goto fail;
	if (pipe(rw2) || fcntl(*rw2, F_SETPIPE_SZ, CAPACITY) != CAPACITY)
		goto fail;

	for (n = CAPACITY; n; n -= (int)r)
		if ((r = write(rw1[1], buf, n)) < 0)
			goto fail;

	if (clock_gettime(CLOCK_MONOTONIC, &start))
		goto fail;
	for (;;) {
		r = splice(rw1[0], NULL, rw2[1], NULL, CAPACITY, 0);
		if (r <= 0) {
			if (r < 0)
				goto fail;
			goto done;
		}
		for (p = 0; p < r; p += w) {
			w = splice(rw2[0], NULL, rw1[1], NULL, r - p, 0);
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
			rate /= 1000000000.;
			rate += start.tv_sec;
			rate = (double)sum / rate;
			rate /= 1000000000.;
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
