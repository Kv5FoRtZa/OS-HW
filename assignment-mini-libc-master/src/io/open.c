// SPDX-License-Identifier: BSD-3-Clause

#include <fcntl.h>
#include <internal/syscall.h>
#include <stdarg.h>
#include <errno.h>

int open(const char *filename, int flags, ...)
{
	//de retinut
	//pt ca sunt bibliotecile care sunt, nu merge return syscall pt ca nu se seteaza errno
	//errno se pune la toate la fel
	int fis = syscall(__NR_open, filename, flags);
	if (fis < 0)
	{
		errno = -fis;
		return -1;
	}///return -1;
	return fis;
}
