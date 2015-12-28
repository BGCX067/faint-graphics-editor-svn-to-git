// Copyright 2012 Lukas Kemmer
//
// Licensed under the Apache License, Version 2.0 (the "License"); you
// may not use this file except in compliance with the License. You
// may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
// implied. See the License for the specific language governing
// permissions and limitations under the License.

#ifndef FAINT_FORMATTING_HH
#define FAINT_FORMATTING_HH
#include <string>
#include "geo/geotypes.hh"

std::string Bracketed( const std::string& );
std::string StrPoint( const Point& );
std::string StrPoint( const IntPoint& );
std::string StrScale( faint::coord scale_x, faint::coord scale_y );
std::string StrLength( faint::coord );
std::string StrLengthInt( int );
std::string StrHex( const faint::Color& );
std::string StrRGB( const faint::Color& );
std::string StrRGBA( const faint::Color& );
std::string StrSmartRGBA(const faint::Color&);
std::string StrSmartRGBA(const faint::Color&, bool prefix);
std::string StrCenterRadius( const Point&, faint::coord rx, faint::coord ry );
std::string StrCenterRadius( const Point&, faint::coord r );
std::string StrFromTo( const Point&, const Point& );
std::string StrFromTo( const IntPoint&, const IntPoint& );
std::string StrAngle( faint::coord );
std::string StrAngle( int );
std::string Percentage( int numerator, int denominator );
std::string StrLineStatus( const Point& p1, const Point& p2 );

#endif
