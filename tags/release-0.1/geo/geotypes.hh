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

#ifndef FAINT_GEOTYPES_HH
#define FAINT_GEOTYPES_HH
#include "basic.hh"
#include "point.hh"
#include "size.hh"
#include "rect.hh"
#include "intsize.hh"
#include "intpoint.hh"
#include "intrect.hh"
#include "line.hh"
#include "arrowhead.hh"
#include "color.hh"
#include <vector>

IntPoint truncated( const Point& ); 
Point floated( const IntPoint& );
Rect floated( const IntRect& );
Size floated( const IntSize& );
IntSize truncated( const Size& );
IntRect truncated( const Rect& );
Rect BoundingRect( const Point&, const Point&, const Point& );
std::vector<Point> Corners( const Rect& );
#endif
