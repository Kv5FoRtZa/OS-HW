// SPDX-License-Identifier: BSD-3-Clause

#include <unistd.h>
#include <internal/syscall.h>
#include <errno.h>

int truncate(const char *path, off_t length)
{
	//acelasi lucru Nr_truncate
	int fis = syscall(__NR_truncate, path, length);
	if (fis < 0)
	{
		errno = -fis;
		return -1;
	}
	return fis;
}
