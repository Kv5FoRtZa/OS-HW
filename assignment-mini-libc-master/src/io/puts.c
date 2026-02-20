// SPDX-License-Identifier: BSD-3-Clause

#include <fcntl.h>
#include <internal/syscall.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
int puts(const char *vct)
{
    //gasim size la string
    //apoi apel de write + eroare
    int size = 0;
    size = strlen(vct);
	int fis = syscall(__NR_write, 1, vct, size);

	if (fis < 0) {
		errno = -fis;
		return -1;
	}
    //1 = stdout
    fis = syscall(__NR_write, 1, "\n", 1);
    	if (fis < 0) {
		errno = -fis;
		return -1;
	}
	return fis;
}
