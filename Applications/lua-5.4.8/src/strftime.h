#ifndef STRFTIME_H
#define STRFTIME_H

size_t strftime(char *s, size_t maxsize, const char *format, const struct tm *timeptr);

#endif