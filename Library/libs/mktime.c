/* origin: https://github.com/Vyjill/tinyutc/blob/main/tinyutc.h */
/**
 * @file tinyutc.h
 * @brief A simple library for converting between Unix timestamps and UTC time.
 * @author Ulysse Moreau
 * @date 2025-05-02
 * @version 2.0
 * @license WTFPL (Do What The F*ck You Want To Public License)
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://www.wtfpl.net/ for more details.
 *
 */
#include <stdio.h>
#include <unistd.h>
#include <time.h>

#define _TINYUTC_UNIX_EPOCH_YEAR (1970UL)
#define _TINYUTC_DAYS_PER_YEAR (365UL)
#define _TINYUTC_DAYS_PER_LEAP_YEAR (366UL)
#define _TINYUTC_SECS_PER_MIN (60UL)
#define _TINYUTC_MIN_PER_HOUR (60UL)
#define _TINYUTC_SECS_PER_HOUR (3600UL)
#define _TINYUTC_HOUR_PER_DAY (24UL)
#define _TINYUTC_SECS_PER_DAY (_TINYUTC_SECS_PER_HOUR * _TINYUTC_HOUR_PER_DAY)
#define _TINYUTC_MONTH_PER_YEAR (12UL)

/**
 * A leap year occurs
 *  - Every 4 years
 *      - Except every 100 years
 *          - Except except every 400 years.
 *
 * This macro returns true :
 * - If the year is divisible by 400
 * - If the year is divisible by 4 and not divisible by 100
 */

#define _TINYUTC_IS_LEAP_YEAR(YEAR) \
    (((YEAR) % 400 == 0) || (((YEAR) % 4 == 0) && ((YEAR) % 100 != 0)))

/**
 * The following macros are used to determine the number of days in a month.
 * This removes the following static declaration that should never appear in a header file:
 *    static const uint8_t days_month[2][12] = {{31, 28, 31, 30, 31, 30,
 *                                              31, 31, 30, 31, 30, 31},
 *                                              {31, 29, 31, 30, 31, 30,
 *                                              31, 31, 30, 31, 30, 31}};
 */

#define _TINYUTC_DAYS_IN_BIG_MONTH 31
#define _TINYUTC_DAYS_IN_SMALL_MONTH 30
#define _TINYUTC_DAYS_IN_FEBRUARY_NON_LEAP 28
#define _TINYUTC_DAYS_IN_FEBRUARY_LEAP 29
// In reverse order, 1 where the month contains 31 days, 0 where it contains 30 days
#define _TINYUTC_DAYS_IN_MONTH_PATTERN 0b101011010101
#define _TINYUTC_GET_DAYS_IN_NON_FEBRUARY(month) \
    ((_TINYUTC_DAYS_IN_MONTH_PATTERN & (1 << (month))) ? _TINYUTC_DAYS_IN_BIG_MONTH : _TINYUTC_DAYS_IN_SMALL_MONTH)

#define _TINYUTC_IS_FEBRUARY(month) ((month) == 1)

#define _TINYUTC_GET_DAYS_IN_FEBRUARY(year) \
    (_TINYUTC_IS_LEAP_YEAR(year) ? _TINYUTC_DAYS_IN_FEBRUARY_LEAP : _TINYUTC_DAYS_IN_FEBRUARY_NON_LEAP)

#define _TINYUTC_GET_DAYS_IN_MONTH(month, year) \
    ((_TINYUTC_IS_FEBRUARY(month)) ? _TINYUTC_GET_DAYS_IN_FEBRUARY(year) : _TINYUTC_GET_DAYS_IN_NON_FEBRUARY(month))


time_t mktime(struct tm * utc_tm) {
    time_t unix_ts = 0;
    tinyutc_utc_to_unix(utc_tm, &unix_ts);
    return unix_ts;
}

/**
* @brief Converts a TinyUTCTime structure to a Unix timestamp.
*
* This function takes a pointer to a TinyUTCTime structure representing
* a date and time in UTC and converts it to the corresponding Unix timestamp.
*
* @param[in] utc_tm Pointer to a TinyUTCTime structure where the converted
*               UTC time will be stored.
* @param[out]  unix_ts The Unix timestamp result.
* @return A uint8_t value indicating success or failure of the conversion.
*         Typically, 0 indicates success, while non-zero values indicate
*         an error.
*/
int tinyutc_utc_to_unix(const struct tm *utc_tm, time_t *unix_ts)
{
    uint8_t days_in_month;
    int i;

    if (utc_tm->tm_year < _TINYUTC_UNIX_EPOCH_YEAR)
    {
        return -1;
    }

    // Offset year is the number of years since the Unix epoch (1970).

    // Start by couting the number of seconds in the year,
    // assuming non-leap year (365 days).
    *unix_ts = (utc_tm->tm_year - _TINYUTC_UNIX_EPOCH_YEAR) * (_TINYUTC_SECS_PER_DAY * _TINYUTC_DAYS_PER_YEAR);

    // For each leap year, add an extra day (86400 seconds).
    for (i = _TINYUTC_UNIX_EPOCH_YEAR; i < utc_tm->tm_year; i++)
    {
        if (_TINYUTC_IS_LEAP_YEAR(i))
        {
            *unix_ts += _TINYUTC_SECS_PER_DAY;
        }
    }

    // For each month, add the number of days in that month.
    for (i = 1; i < utc_tm->tm_mon; i++)
    {
        days_in_month = _TINYUTC_GET_DAYS_IN_MONTH(i - 1, utc_tm->tm_year);
        *unix_ts += _TINYUTC_SECS_PER_DAY * days_in_month;
    }

    // The remaining is trivial:
    // days, hours, minutes and seconds.
    *unix_ts += (utc_tm->tm_mday - 1) * _TINYUTC_SECS_PER_DAY;
    *unix_ts += utc_tm->tm_hour * _TINYUTC_SECS_PER_HOUR;
    *unix_ts += utc_tm->tm_min * _TINYUTC_SECS_PER_MIN;
    *unix_ts += utc_tm->tm_sec;

    return 0;
}
