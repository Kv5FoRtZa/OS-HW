// SPDX-License-Identifier: BSD-3-Clause

#include <unistd.h>
#include <internal/syscall.h>
#include <errno.h>

int ftruncate(int fd, off_t length)
{
	int fis = syscall(__NR_ftruncate, fd, length);
	if (fis < 0)
	{
		errno = -fis;
		return -1;
	}
	return fis;
}
