// util.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// Miscellaneous useful functions.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _UTIL_H_
#define _UTIL_H_

#include <string>
#include <vector>
#include <iostream>
#include <functional>

// A little trickery to get something like a compile time assert in C++
#define COMPILE_TIME_ASSERT(pred) \
    switch(0){case 0: case pred:;}

#ifdef _WIN32
// The Windows header files define min and max macros. We prefer the min and
// max templates from STL because they don't result in unexpected multiple
// evaluations of arguments. In order to use them, we need to set NOMINMAX
// to prevent namespace pollution by the Windows macros.
#define NOMINMAX
#endif
				      
#ifdef _WIN32
#include "libintl.h"
#define _(string) gettext (string)
#else
#define _(string) gettext (string)
#endif

extern int compareIgnoringCase(const std::string& s1, const std::string& s2);
extern int compareIgnoringCase(const std::string& s1, const std::string& s2, int n);
extern std::string LocaleFilename(const std::string & filename);

class CompareIgnoringCasePredicate : public std::binary_function<std::string, std::string, bool>
{
 public:
    bool operator()(const std::string&, const std::string&) const;
};

template <class T> struct printlineFunc : public std::unary_function<T, void>
{
    printlineFunc(std::ostream& o) : out(o) {};
    void operator() (T x) { out << x << '\n'; };
    std::ostream& out;
};

template <class T> struct deleteFunc : public std::unary_function<T, void>
{
    deleteFunc() {};
    void operator() (T x) { delete x; };
    int dummy;
};

#endif // _UTIL_H_
