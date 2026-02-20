// SPDX-License-Identifier: BSD-3-Clause

#include <unistd.h>
#include <internal/syscall.h>
#include <errno.h>

off_t lseek(int fd, off_t offset, int whence)
{
	//off_t se asigura ca returnul returneaza tipul corect de file
	int fis = syscall(__NR_lseek, fd, offset, whence);
	if (fis < 0)
	{
		errno = -fis;
		return (off_t)-1;
	}
	return (off_t)fis;
}
