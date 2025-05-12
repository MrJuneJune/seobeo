#include <seobeo/helper.h>

void GetTimeStamp(char* time_stamp, size_t size) {
  time_t now = time(NULL);
  struct tm *tm_info = localtime(&now);
  strftime(time_stamp, size, "%Y-%m-%d %H:%M:%S", tm_info);
}
