/*******************************************************************************
*  file    : time_utils.hpp
*  created : 01.08.2018
*  author  : Slyshyk Oleksiy (alexslyshyk@gmail.com)
*******************************************************************************/
#ifndef TIME_UTILS_HPP
#define TIME_UTILS_HPP

#include <ctime>

namespace ldlog
{
namespace details
{
inline std::tm localtime(const std::time_t &time_tt)
{
#ifdef _WIN32
    std::tm tm;
    localtime_s(&tm, &time_tt);
#else
    std::tm tm;
    localtime_r(&time_tt, &tm);
#endif
    return tm;
}

inline std::tm localtime()
{
    std::time_t now_t = time(nullptr);
    return localtime(now_t);
}

inline std::tm gmtime(const std::time_t &time_tt)
{

#ifdef _WIN32
    std::tm tm;
    gmtime_s(&tm, &time_tt);
#else
    std::tm tm;
    gmtime_r(&time_tt, &tm);
#endif
    return tm;
}

inline std::tm gmtime()
{
    std::time_t now_t = time(nullptr);
    return gmtime(now_t);
}
inline bool operator==(const std::tm &tm1, const std::tm &tm2)
{
    return (tm1.tm_sec == tm2.tm_sec && tm1.tm_min == tm2.tm_min && tm1.tm_hour == tm2.tm_hour && tm1.tm_mday == tm2.tm_mday &&
            tm1.tm_mon == tm2.tm_mon && tm1.tm_year == tm2.tm_year && tm1.tm_isdst == tm2.tm_isdst);
}

inline bool operator!=(const std::tm &tm1, const std::tm &tm2)
{
    return !(tm1 == tm2);
}

}//namespace details
} //namespace ldlog

#endif // TIME_UTILS_HPP
