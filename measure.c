/* See LICENSE file for copyright and license details. */
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
	int fd, n = 0, m = 150000;
	ssize_t r;
	size_t sum = 0, totsum = 0;
	struct timespec start, end;
	double rate;

	if (!argc)
		*argv = "measure";

	if (argc > 1)
		m = atoi(argv[1]);

	fd = open("/dev/null", O_WRONLY);
	if (fd < 0)
		goto fail;

	usleep(10000L);
	if (argc < 2 && fcntl(STDIN_FILENO, F_GETPIPE_SZ) == CAPACITY)
		m = 10;

	if (clock_gettime(CLOCK_MONOTONIC, &start))
		goto fail;
	for (;;) {
		r = splice(STDIN_FILENO, NULL, fd, NULL, SSIZE_MAX, 0);
		if (r <= 0) {
			if (r < 0)
				goto fail;
			goto done;
		}
		sum += (size_t)r;
		if (++n == m) {
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
