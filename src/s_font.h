/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

// Class to manage s_fonts

#ifndef _S_FONT_H
#define _S_FONT_H

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "SDL_opengl.h"

#include "stel_utility.h"
#include "s_texture.h"
#include "typeface.h"

class s_font
{
public:
    s_font(float size_i, const string& ttfFileName) : typeFace(ttfFileName, (size_t)(size_i), 72) {;}
    virtual ~s_font() {;}
    void print(float x, float y, const wstring& s, int upsidedown = 1) {typeFace.render(s, Vec2f(x, y), upsidedown==1);}
    void print(float x, float y, const string& s, int upsidedown = 1) {typeFace.render(StelUtility::stringToWstring(s), Vec2f(x, y), upsidedown==1);}
    void print_char(const unsigned char c) {wchar_t wc = c; typeFace.renderGlyphs((wstring(&wc)));}
    void print_char_outlined(const unsigned char c) {wchar_t wc = c; typeFace.renderGlyphs((wstring(&wc)));}
    float getStrLen(const wstring& s) {return typeFace.width(s);}
    float getStrLen(const string& s) {return typeFace.width(StelUtility::stringToWstring(s));}
    float getLineHeight(void) {return typeFace.lineHeight();}
    float getAscent(void) {return typeFace.ascent();}
    float getDescent(void) {return typeFace.descent();}
    static wstring strToWstring(const string& s) {wstring ws(s.begin(), s.end()); ws.assign(s.begin(), s.end()); return ws;}
protected:
	TypeFace typeFace;
	
};


#endif  //_S_FONT_H
