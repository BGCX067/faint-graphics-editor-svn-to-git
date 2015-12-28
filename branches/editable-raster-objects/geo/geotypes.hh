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
#include "radii.hh"
#include "range.hh"
#include "size.hh"
#include "scale.hh"
#include "rect.hh"
#include "intsize.hh"
#include "intpoint.hh"
#include "intrect.hh"
#include "line.hh"
#include "arrowhead.hh"
#include <vector>

std::vector<IntPoint> as_line_path( const IntRect& );
// Same as IntRect(const IntPoint&, const IntPoint&), but
// sometimes looks more consistent
IntRect bounding_rect( const IntPoint&, const IntPoint& );
IntRect bounding_rect( const std::vector<IntPoint>& );
Rect bounding_rect( const Point&, const Point&, const Point& );
Rect bounding_rect( const std::vector<Point>& );
std::vector<Point> corners( const Rect& );
std::vector<Point> points( const Tri& );

// Single-value point construction
IntPoint delta_x(int);
Point delta_x(faint::coord);
IntPoint delta_xy( int x, int y );
Point delta_xy(faint::coord, faint::coord);
IntPoint delta_y(int);
Point delta_y(faint::coord);

Point floated( const IntPoint& );
Rect floated( const IntRect& );
Size floated( const IntSize& );
Point point_from_size( const Size& );
IntPoint point_from_size( const IntSize& );
Radii radii_from_point( const Point& );
IntRect rect_from_size( const IntSize& );
PathPt rotate_point( const PathPt&, faint::radian, const Point& origin );
IntPoint rounded( const Point& );
IntSize rounded( const Size& );

// Floors the left, top, ceils the right and bottom, so that the
// IntRect includes all pixels the Rect intersects.
IntRect floiled( const Rect& );

PathPt scale_point( const PathPt&, const Scale&, const Point& origin );
IntSize size_from_point( const IntPoint& );
IntSize truncated( const Size& );
int floored( faint::coord );
IntPoint floored( const Point& );
IntRect floored( const Rect& );
IntSize floored( const Size& );

// Converts to size_t. Asserts if < 0
size_t to_size_t(int);
size_t to_size_t(size_t); // undefined

// Conversion from size_t to int. For idiosyncratic reasons, I'm
// trying to migrate to using size_t less, using this conversion
// instead of a raw static_cast will point me to conversions that are
// no longer required.
int resigned( size_t );
int resigned( int ); // ...thanks to this undefined overload.
int resigned( faint::coord ); // also undefined.

#endif
