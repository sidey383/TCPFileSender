#pragma once
#include <sys/time.h>

unsigned long getMicroseconds(struct timeval *start, struct timeval *end);

std::string getTimeDifStr(struct timeval *start, struct timeval *end);

std::string getSpeedStr(size_t bitCount, struct timeval *start, struct timeval *end) ;