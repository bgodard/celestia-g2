// stellarclass.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include <cstring>
#include <cstdio>
#include <cassert>
#include "celestia.h"
#include "stellarclass.h"

using namespace std;


Color StellarClass::getApparentColor() const
{
    switch (getSpectralClass())
    {
    case Spectral_O:
        return Color(0.7f, 0.8f, 1.0f);
    case Spectral_B:
        return Color(0.8f, 0.9f, 1.0f);
    case Spectral_A:
        return Color(1.0f, 1.0f, 1.0f);
    case Spectral_F:
        return Color(1.0f, 1.0f, 0.88f);
    case Spectral_G:
        return Color(1.0f, 1.0f, 0.75f);
    case StellarClass::Spectral_K:
        return Color(1.0f, 0.9f, 0.7f);
    case StellarClass::Spectral_M:
        return Color(1.0f, 0.7f, 0.7f);
    case StellarClass::Spectral_R:
    case StellarClass::Spectral_S:
    case StellarClass::Spectral_N:
        return Color(1.0f, 0.6f, 0.6f);
    default:
        // TODO: Figure out reasonable colors for Wolf-Rayet stars,
        // white dwarfs, and other oddities
        return Color(1.0f, 1.0f, 1.0f);
    }
}


// The << method of converting the stellar class to a string is
// preferred, but it's not always practical, especially when you've
// got a completely broken implemtation of stringstreams to
// deal with (*cough* gcc *cough*).
//
// Return the buffer if successful or NULL if not (the buffer wasn't
// large enough.)
char* StellarClass::str(char* buf, unsigned int buflen) const
{
    StellarClass::StarType st = getStarType();
    char s0[3];
    char s1[2];
    char* s2 = "";
    s0[0] = '\0';
    s1[0] = '\0';

    if (st == StellarClass::WhiteDwarf)
    {
        strcpy(s0, "WD");
    }
    else if (st == StellarClass::NeutronStar)
    {
        strcpy(s0, "Q");
    }
    else if (st == StellarClass::NormalStar)
    {
	s0[0] = "OBAFGKMRSNWW?"[(unsigned int) getSpectralClass()];
        s0[1] = '\0';
	s1[0] = "0123456789"[getSpectralSubclass()];
        s1[1] = '\0';
	switch (getLuminosityClass())
        {
	case StellarClass::Lum_Ia0:
	    s2 = " I-a0";
	    break;
	case StellarClass::Lum_Ia:
	    s2 = " I-a";
	    break;
	case StellarClass::Lum_Ib:
	    s2 = " I-b";
	    break;
	case StellarClass::Lum_II:
	    s2 = " II";
	    break;
	case StellarClass::Lum_III:
	    s2 = " III";
	    break;
	case StellarClass::Lum_IV:
	    s2 = " IV";
	    break;
	case StellarClass::Lum_V:
	    s2 = " V";
	    break;
	case StellarClass::Lum_VI:
	    s2 = " VI";
	    break;
	}
    }
    else
    {
        strcpy(s0, "?");
    }

    if (strlen(s0) + strlen(s1) + strlen(s2) >= buflen)
    {
        return NULL;
    }
    else
    {
        sprintf(buf, "%s%s%s", s0, s1, s2);
        return buf;
    }
}


string StellarClass::str() const
{
    char buf[20];
    str(buf, sizeof buf);
    return string(buf);
}


ostream& operator<<(ostream& os, const StellarClass& sc)
{
    char buf[20];
    char *scString = sc.str(buf, sizeof buf);
    assert(scString != NULL);

    os << scString;

    return os;
}


bool operator<(const StellarClass& sc0, const StellarClass& sc1)
{
    return sc0.data < sc1.data;
}
