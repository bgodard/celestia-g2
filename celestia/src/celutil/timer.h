// timer.h
// 
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _TIMER_H_
#define _TIMER_H_

class Timer
{
 public:
    Timer() {};
    virtual ~Timer() {};
    virtual double getTime() const = 0;
    virtual void reset() = 0;
};

extern Timer* CreateTimer();

#endif // _TIMER_H_
