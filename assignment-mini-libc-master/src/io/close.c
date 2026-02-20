// SPDX-License-Identifier: BSD-3-Clause

#include <unistd.h>
#include <internal/syscall.h>
#include <stdarg.h>
#include <errno.h>

int close(int fd)
{
	//close da -1 sau 0 daca e succes sau nu
	//errno ca la open
	int fis = syscall(__NR_close, fd);
	if (fis < 0)
	{
		errno = -fis;
		return -1;
	}
	return 0;
}
