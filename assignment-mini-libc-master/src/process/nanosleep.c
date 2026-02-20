#include <sys/stat.h>
#include <internal/syscall.h>
#include <internal/types.h>
#include <time.h>
#include <errno.h>
//dur = cat face sleep(5 ore juma)
//rem = timpul ramas dintre alarma si ora cand m-as fi trezit
int nanosleep(const struct timespec *dur, struct timespec *rem)
{
int fis = syscall(__NR_nanosleep, dur, rem);
	if (fis < 0) {
		errno = -fis;
		return -1;
	}
	return fis;
}
