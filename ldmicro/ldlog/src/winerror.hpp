/*******************************************************************************
*  file    : winerror.hpp
*  created : 08.08.2018
*  author  : Slyshyk Oleksiy (alexslyshyk@gmail.com)
*******************************************************************************/
#ifndef WINERROR_HPP
#define WINERROR_HPP

#include <string>
#include "ldlogexport.h"

namespace ldlog
{

LDLOG_API std::string strwinerr();
LDLOG_API std::wstring wstrwinerr();

} //namespace ldlog

#endif // WINERROR_HPP
