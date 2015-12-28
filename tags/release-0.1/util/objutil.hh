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

#ifndef FAINT_OBJUTIL_HH
#define FAINT_OBJUTIL_HH
class Grid;
inline bool RectHitTest( const Point& p, int x, int y, int w, int h ){
  return !( p.x < x || p.y < y || ( x + w ) < p.x ||
    ( y + h ) < p.y );
}

sides RectshapedHandleIndex( const Point& p, int x, int y, int w, int h );
void RectshapedAdjust( int handleIndex, Point p, faint::coord& x, faint::coord& y, faint::coord& w, faint::coord& h );
extern const faint::coord g_maxSnapDistance;
Point Snap( const Point&, const std::vector<Object*>&, const Grid&, faint::coord maxDistance=g_maxSnapDistance);
faint::coord SnapX( faint::coord x, const std::vector<Object*>&, const Grid&, faint::coord y0, faint::coord y1, faint::coord maxDistance=g_maxSnapDistance );
faint::coord SnapY( faint::coord y, const std::vector<Object*>&, const Grid&, faint::coord x0, faint::coord x1, faint::coord maxDistance=g_maxSnapDistance );
FaintSettings DebugRectSettings();
Rect BoundingRect( const std::vector<Object*>& );
void Offset( const std::vector<Object*>&, const Point& );
void AttachPointsFromTri( const Tri&, std::vector<Point>& );
std::vector<Point> AttachPointsFromTri( const Tri& );
std::vector<Object*> Clone( const std::vector<Object*>& objects );

#endif
