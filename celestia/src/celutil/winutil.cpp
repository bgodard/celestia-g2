// winutil.h
// 
// Copyright (C) 2002, Chris Laurel <claurel@shatters.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "winutil.h"

void SetMouseCursor(LPCTSTR lpCursor)
{
	HCURSOR hNewCrsr;
	
	if (hNewCrsr = LoadCursor(NULL, lpCursor))
	    SetCursor(hNewCrsr);
}

void CenterWindow(HWND hParent, HWND hWnd)
{
    //Center window with hWnd handle relative to hParent.
    if (hParent && hWnd)
    {
        RECT or, ir;
        if (GetWindowRect(hParent, &or))
        {
            if (GetWindowRect(hWnd, &ir))
            {
                int x, y;

                x = or.left + (or.right - or.left - (ir.right - ir.left)) / 2;
                y = or.top + (or.bottom - or.top - (ir.bottom - ir.top)) / 2;;
                SetWindowPos(hWnd, HWND_TOP, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
            }
        }
    }
}

void RemoveButtonDefaultStyle(HWND hWnd)
{
    SetWindowLong(hWnd, GWL_STYLE,
		::GetWindowLong(hWnd, GWL_STYLE) & ~BS_DEFPUSHBUTTON);
	InvalidateRect(hWnd, NULL, TRUE);
}

void AddButtonDefaultStyle(HWND hWnd)
{
    SetWindowLong(hWnd, GWL_STYLE,
        ::GetWindowLong(hWnd, GWL_STYLE) | BS_DEFPUSHBUTTON);
	InvalidateRect(hWnd, NULL, TRUE);
}