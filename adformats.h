#ifndef INCLUDED_ADFORMATS_H
#define INCLUDED_ADFORMATS_H

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

const char* format_time (time_t timestamp);
time_t timestamp2time (const char* timestamp);
const char* time2timestamp (time_t timestamp);
const char* format_timestamp (const char* timestamp);
time_t timevalue2time (long long timevalue);
time_t timeint2time (long long timevalue);
long long time2timevalue_n (time_t timestamp);
const char* time2timevalue (time_t timestamp);
const char* format_timevalue (long long timevalue);
const char* get_primary_smtp_address (char* proxyaddress);
const char* replace_line_breaks_with_spaces (char* data);

#ifdef __cplusplus
}
#endif

#endif //INCLUDED_ADFORMATS_H
