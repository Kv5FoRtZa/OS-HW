// SPDX-License-Identifier: BSD-3-Clause

#include <sys/stat.h>
#include <internal/types.h>
#include <unistd.h>
#include <internal/syscall.h>
#include <fcntl.h>
#include <errno.h>

#define makedev(x, y) ( \
		(((x)&0xfffff000ULL) << 32) | \
	(((x)&0x00000fffULL) << 8) | \
		(((y)&0xffffff00ULL) << 12) | \
	(((y)&0x000000ffULL)) \
	)

/* Structure describing file characteristics as defined in linux/stat.h */
struct statx {
	uint32_t stx_mask;
	uint32_t stx_blksize;
	uint64_t stx_attributes;
	uint32_t stx_nlink;
	uint32_t stx_uid;
	uint32_t stx_gid;
	uint16_t stx_mode;
	uint16_t pad1;
	uint64_t stx_ino;
	uint64_t stx_size;
	uint64_t stx_blocks;
	uint64_t stx_attributes_mask;
	struct {
		int64_t tv_sec;
		uint32_t tv_nsec;
		int32_t pad;
	} stx_atime, stx_btime, stx_ctime, stx_mtime;
	uint32_t stx_rdev_major;
	uint32_t stx_rdev_minor;
	uint32_t stx_dev_major;
	uint32_t stx_dev_minor;
	uint64_t spare[14];
};
//makedev este un macro helper care interpreteaza/ construieste device ID
//statx = tot stat/fstatat care te lasa sa alegi ce campuri vrei pt performanta
int fstatat_statx(int fd, const char *restrict path, struct stat *restrict st, int flag)
{
	int fis = syscall(__NR_newfstatat, fd, path, st, flag);
	if (fis < 0)
	{
		errno = -fis;
		return -1;
	}
	return fis;
}

int fstatat(int fd, const char *restrict path, struct stat *restrict st, int flag)
{
	//int fis = syscall(__NR_newfstatat,fd,path,st,flag);
	//if (fis < 0)
	{
		//errno = -fis;
		//return -1;
	}
	int fis = fstatat_statx(fd, path, st, flag);
	return fis;
}
