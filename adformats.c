#include "adformats.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

const char* format_time (time_t timestamp)
{
  if (timestamp == 0)
    return "";
#if 0
  static char result[26];
  struct tm* timedata = gmtime(&timestamp);
  strftime(result, 26, "%Y-%m-%d %H:%M:%S (UTC)", timedata);
#else
  static char result[20];
  struct tm* timedata = localtime(&timestamp);
  strftime(result, 20, "%Y-%m-%d %H:%M:%S", timedata);
#endif
  return result;
}

time_t timestamp2time (const char* timestamp)
{
  if (!timestamp || !*timestamp || strcmp(timestamp, "0") == 0)
    return 0;
  struct tm timedata;
  sscanf(timestamp, "%4d%2d%2d%2d%2d%2d", &timedata.tm_year, &timedata.tm_mon, &timedata.tm_mday, &timedata.tm_hour, &timedata.tm_min, &timedata.tm_sec);
  timedata.tm_year -= 1900;
  timedata.tm_mon -= 1;
  timedata.tm_wday = 0;
  timedata.tm_yday = 0;
  timedata.tm_isdst = 0;
  return mktime(&timedata);
}

const char* time2timestamp (time_t timestamp)
{
  static char result[18];
  struct tm* timedata = localtime(&timestamp);
  strftime(result, 20, "%Y%m%d%H%M%S.0Z", timedata);
  return result;
}

const char* format_timestamp (const char* timestamp)
{
  return format_time(timestamp2time(timestamp));
}

time_t timevalue2time (long long timevalue)
{
  /* timevalue: 64-bit number representing the number of 100-nanosecond intervals since 1601-01-01 00:00 */
  if (timevalue == 0 || timevalue == 0x7FFFFFFFFFFFFFFFLL)
    return 0;
  return timevalue / 10000000 - (long long)((1970 - 1601) * 365 + 89) * 24 * 60 * 60;
}

time_t timeint2time (long long timevalue)
{
  struct tm timedetails;
  if (timevalue <= 0)
    return 0;
  timedetails.tm_year = (timevalue / 10000000000) % 10000 - 1900;
  timedetails.tm_mon = (timevalue / 100000000) % 100 - 1;
  timedetails.tm_mday = (timevalue / 1000000) % 100;
  timedetails.tm_hour = (timevalue / 10000) % 100;
  timedetails.tm_min = (timevalue / 100) % 100;
  timedetails.tm_sec = timevalue % 100;
  timedetails.tm_yday = -1;
  timedetails.tm_wday = -1;
  timedetails.tm_isdst = -1;
  return mktime(&timedetails);
}

long long time2timevalue_n (time_t timestamp)
{
  if (timestamp == 0)
    return 0;
  return ((long long)timestamp + (long long)((1970 - 1601) * 365 + 89) * 24 * 60 * 60) * 10000000;
}

const char* time2timevalue (time_t timestamp)
{
  static char buf[32];
//#ifdef _WIN32
//  lltoa(time2timevalue_n(timestamp), buf, 10);
//#else
  snprintf(buf, sizeof(buf), "%" PRIi64, (int64_t)time2timevalue_n(timestamp));
//#endif
  return buf;
}

const char* format_timevalue (long long timevalue)
{
  return format_time(timevalue2time(timevalue));
}

const char* get_primary_smtp_address (char* proxyaddress)
{
  char* linestart;
  char* line = proxyaddress;
  while (line && *line) {
    linestart = line;
    while (*line) {
      if (*line++ == '\n') {
        break;
      }
    }
    if (strncmp(linestart, "SMTP:", 5) == 0) {
      if (line[-1] == '\n')
        line[-1] = 0;
      return linestart + 5;
    }
  }
  return NULL;
}

const char* replace_line_breaks_with_spaces (char* data)
{
  if (data) {
    char* p = data;
    while (*p) {
      if (*p == '\r' || *p == '\n')
        *p = ' ';
      p++;
    }
  }
  return data;
}

