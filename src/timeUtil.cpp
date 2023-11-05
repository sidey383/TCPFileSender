#include <iomanip>
#include <sys/time.h>
#include "timeUtil.hpp"

unsigned long getMicroseconds(struct timeval *start, struct timeval *end){
    return ((end->tv_sec - start->tv_sec) * 1000000 + end->tv_usec - start->tv_usec);
}

std::string getTimeDifStr(struct timeval *start, struct timeval *end) {
    std::ostringstream oss;
    __time_t sec = 0;
    __suseconds_t usec = 0;
    if (end->tv_usec < start->tv_usec) {
        sec = end->tv_sec - start->tv_sec - 1;
        usec = 1000000 + end->tv_usec - start->tv_usec;
    } else {
        sec = end->tv_sec - start->tv_sec;
        usec = end->tv_usec - start->tv_usec;
    }
        oss << sec << "." << std::setfill('0') << std::setw(5) << usec;
    return oss.str();
}

std::string getSpeedStr(size_t bitCount, struct timeval *start, struct timeval *end) {
    double kBitSpeed = (double) bitCount * 1000000.0 / ((double) getMicroseconds(start, end) * 1024);
    std::ostringstream oss;
    if (kBitSpeed < 4096) {
        oss << kBitSpeed << " Kbit/s";
    } else {
        oss << kBitSpeed / 1024 << " Mbit/s";
    }
    return oss.str();
}