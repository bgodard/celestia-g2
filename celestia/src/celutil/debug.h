// debug.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _DEBUG_H_
#define _DEBUG_H_

// Define the DPRINTF macro; g++ supports macros with variable
// length arguments, so we'll use those when we can.
#ifdef __GNUC__

#ifndef DEBUG
#define DPRINTF(int level, args...)
#else
#define DPRINTF(int level, args...) DebugPrint(level, args)
extern void DebugPrint(int level, char *format, ...);
#endif

#else

#ifndef _DEBUG
#define DPRINTF //
#else
#define DPRINTF DebugPrint
extern void DebugPrint(int level, char *format, ...);
#endif

#endif

extern void SetDebugVerbosity(int);
extern int GetDebugVerbosity();

#endif // _DEBUG_H_

