// SPDX-License-Identifier: BSD-3-Clause

#include <sys/stat.h>
#include <internal/syscall.h>
#include <errno.h>

int fstat(int fd, struct stat *st)
{
	int fis = syscall(__NR_fstat, fd, st);
	if (fis < 0)
	{
		errno = -fis;
		return -1;
	}
	return fis;
}
