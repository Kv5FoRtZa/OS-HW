#include <unistd.h>
#include <internal/syscall.h>
#include <internal/types.h>
#include <time.h>

unsigned int sleep(unsigned int dur)
{
    struct timespec warning_linter;
    warning_linter.duratasec = dur;
    warning_linter.duratananosec = 0;
    int zero = 0;//cat vreau sa se opreasca early or some thing
    int ceva_rezultat = nanosleep(&warning_linter, NULL);
    if (ceva_rezultat == 0)
    {
        return 0;
    }
	return zero;
}
