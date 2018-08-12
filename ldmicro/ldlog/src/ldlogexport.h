/*******************************************************************************
*  file    : ldlogexport.h
*  created : 08.08.2018
*  author  : Slyshyk Oleksiy (alexslyshyk@gmail.com)
*******************************************************************************/
#ifndef LDLOGEXPORT_H
#define LDLOGEXPORT_H

#if defined(_WIN32)
#ifdef LDLOG_EXPORT
#define LDLOG_API __declspec(dllexport)
#elif defined(LDLOG_SHARED)
#define LDLOG_API __declspec(dllimport)
#endif
#endif
#ifndef LDLOG_API
#define LDLOG_API
#endif

#endif // LDLOGEXPORT_H
