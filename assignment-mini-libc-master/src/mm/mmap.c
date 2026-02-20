// SPDX-License-Identifier: BSD-3-Clause

#include <sys/mman.h>
#include <errno.h>
#include <internal/syscall.h>

void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
{
	//mapeaza da load la memorie
	long fis = syscall(__NR_mmap, addr, length, prot, flags, fd, offset);
	if (fis < 0)
	{
		errno = -fis;
		return MAP_FAILED;
	}
	return ((void*)fis);
}

void *mremap(void *old_address, size_t old_size, size_t new_size, int flags)
{
	//resize the map aka mai multa memorie mapata
	long fis = syscall(__NR_mremap, old_address, old_size, new_size, flags);
	if (fis < 0)
	{
		errno = -fis;
		//((void*)-1) = MAP_FAILED
		return MAP_FAILED;
	}
	return ((void*)fis);
}

int munmap(void *addr, size_t length)
{
	//elibereaza memoria mapata
	int fis = syscall(__NR_munmap, addr, length);
	if (fis < 0)
	{
		errno = -fis;
		return -1;
	}
	return fis;
}
