#ifndef TIME_H
#define TIME_H
struct timespec{
    long duratasec;
    long duratananosec;
};
int nanosleep(const struct timespec *dur, struct timespec *rem);
#endif
