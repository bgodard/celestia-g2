// overlay.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _OVERLAY_H_
#define _OVERLAY_H_

#include <string>
#include <iostream>
#include "texturefont.h"


class Overlay;

// Custom streambuf class to support C++ operator style output.  The
// output is completely unbuffered so that it can coexist with printf
// style output which the Overlay class also supports.
class OverlayStreamBuf : public std::streambuf
{
 public:
    OverlayStreamBuf() : overlay(NULL) { setbuf(0, 0); };

    void setOverlay(Overlay*);

    int overflow(int c = EOF);

 private:
    Overlay* overlay;
};


class Overlay : public std::ostream
{
 public:
    Overlay();
    ~Overlay();

    void begin();
    void end();

    void setWindowSize(int, int);
    void setFont(TextureFont*);

    void rect(float x, float y, float w, float h);

    void beginText();
    void endText();
    void print(char);
    void print(char*);
    void printf(char*, ...);
    

 private:
    int windowWidth;
    int windowHeight;
    TextureFont* font;
    bool useTexture;
    int textBlock;

    OverlayStreamBuf sbuf;
};

#endif // _OVERLAY_H_
