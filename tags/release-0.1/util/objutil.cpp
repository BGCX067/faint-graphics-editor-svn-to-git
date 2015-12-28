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

#include "object.hh"
#include "objutil.hh"
#include "objrectangle.hh"
#include "util/util.hh"
#include "geo/grid.hh"

sides RectshapedHandleIndex( const Point& p, int x, int y, int w, int h ){
  Rect box( floated(IntPoint(x, y)), floated(IntSize(w,h)) );
  Rect larger = Inflated(box, 5);
  if ( ! larger.Contains( p ) ){
    return INVALID;
  }
  if ( RectHitTest( p, x -5, y -5, 10, 10 ) ){
    return TOP_LEFT;
  }
  if ( RectHitTest( p, x + w - 5, y - 5, 10, 10 ) ){
    return TOP_RIGHT;
  }
  if ( RectHitTest(p, x - 5, y + h - 5, 10, 10 ) ){
    return BOTTOM_LEFT;
  }
  if ( RectHitTest( p, x + w - 5, y + h - 5, 10, 10 ) ){
    return BOTTOM_RIGHT;
  }
  return INVALID;
}

void RectshapedAdjust( int handleIndex, Point p, faint::coord& x, faint::coord& y, faint::coord& w, faint::coord& h ) {
  if ( handleIndex == TOP_LEFT ){
    p.x = std::min( p.x, x + w - 1 );
    p.y = std::min( p.y, y + h - 1 );
    w -= p.x - x;
    h -= p.y - y;
    x = p.x;
    y = p.y;
   }
   else if ( handleIndex == TOP_RIGHT ){
     p.x = std::max( p.x,
       x + 1 );
     p.y = std::min( p.y,
       y + h - 1 );

    p.y = std::min( p.y,
      y + h - 1 );
    w = p.x - x;
    h -= p.y - y;
    y = p.y;

  }
  else if ( handleIndex == BOTTOM_LEFT ){
    p.x = std::min( p.x,
      x + w );
    p.y = std::max( p.y,
      y + 1 );

    w = ( x - p.x ) + w;
    x = p.x;
    h = p.y - y;
  }
  else if ( handleIndex == BOTTOM_RIGHT ){
    p.x = std::max( p.x,
      x + 1 );

    p.y = std::max( p.y,
      y + 1 );
    w = p.x - x;
    h = p.y - y;
  }
}

const faint::coord g_maxSnapDistance = LITCRD(20.0);

Point Snap( const Point& sourcePt, const std::vector<Object*>& objects, const Grid& grid, faint::coord maxSnapDistance ){
  faint::coord lastSnapDistance = maxSnapDistance;
  Point currentPt( sourcePt );
  for ( size_t i_obj = 0; i_obj!= objects.size(); i_obj++ ){
    std::vector<Point> points = objects[i_obj]->GetAttachPoints();
    Point snapPt(0,0);

    for ( size_t i_pt = 0; i_pt!= points.size(); i_pt++ ){
      snapPt = points[i_pt];
      faint::coord snapDistance = distance(sourcePt, snapPt);
      if ( snapDistance < lastSnapDistance ){
        // Snap to this closer point instead
        lastSnapDistance = snapDistance;
        currentPt = snapPt;
      }
    }
    if ( currentPt == snapPt ){
      // Fixme: I'm not sure what this is about? Is it to prefer snapping to the topmost object?
      break;
    }
  }

  if ( grid.Enabled() ){
    Point gridPoint = Snap( grid, sourcePt );
    faint::coord snapDistance = distance(sourcePt, gridPoint);
    if ( snapDistance < lastSnapDistance ){
      currentPt = gridPoint;
    }
  }

  return currentPt;
}

faint::coord SnapX( faint::coord sourceX, const std::vector<Object*>& objects, const Grid& grid, faint::coord y0, faint::coord y1, faint::coord maxSnapDistance ){
  faint::coord lastSnapDistance = maxSnapDistance;
  faint::coord current( sourceX );
  for ( size_t i_obj = 0; i_obj!= objects.size(); i_obj++ ){
    std::vector<Point> points = objects[i_obj]->GetAttachPoints();

    Point snapPt(0,0);
    for ( size_t i_pt = 0; i_pt!= points.size(); i_pt++ ){
      snapPt = points[i_pt];
      if ( y0 <= snapPt.y && snapPt.y <= y1 ){
        faint::coord snapDistance = std::fabs(snapPt.x - sourceX );
        if ( lastSnapDistance > snapDistance ){
          // Snap to this closer point instead
          lastSnapDistance = snapDistance;
          current = snapPt.x;
        }
      }
    }
    if ( current == snapPt.x ){
      // Fixme: I'm not sure what this is about? Is it to prefer snapping to the topmost object?
      break;
    }
  }

  if ( grid.Enabled() ){
    Point gridPoint = Snap( grid, Point( sourceX, 0 ) );
    faint::coord snapDistance = abs( sourceX - gridPoint.x );
    if ( snapDistance < lastSnapDistance ){
      current = gridPoint.x;
    }
  }

  return current;
}

faint::coord SnapY( faint::coord sourceY, const std::vector<Object*>& objects, const Grid& grid, faint::coord x0, faint::coord x1, faint::coord maxSnapDistance ){
  faint::coord lastSnapDistance = maxSnapDistance;
  faint::coord current( sourceY );
  for ( size_t i_obj = 0; i_obj!= objects.size(); i_obj++ ){
    std::vector<Point> points = objects[i_obj]->GetAttachPoints();

    Point snapPt(0,0);
    for ( size_t i_pt = 0; i_pt!= points.size(); i_pt++ ){
      snapPt = points[i_pt];
      if ( x0 <= snapPt.x && snapPt.x <= x1 ){
        faint::coord snapDistance = std::fabs(snapPt.y - sourceY );
        if ( lastSnapDistance > snapDistance ){
          // Snap to this closer point instead
          lastSnapDistance = snapDistance;
          current = snapPt.y;
        }
      }
    }
    if ( current == snapPt.y ){
      // Fixme: I'm not sure what this is about? Is it to prefer snapping to the topmost object?
      break;
    }
  }
  if ( grid.Enabled() ){
    Point gridPoint = Snap( grid, Point( 0, sourceY ) );
    faint::coord snapDistance = fabs( sourceY - gridPoint.y );
    if ( snapDistance < lastSnapDistance ){
      current = gridPoint.y;
    }
  }
  return current;
}

Rect BoundingRect( const std::vector<Object*>& objects ){
  assert( !objects.empty() );
  Tri t(objects[0]->GetTri());
  Point minPt = MinCoords(t.P0(), t.P1(), t.P2(), t.P3());
  Point maxPt = MaxCoords(t.P0(), t.P1(), t.P2(), t.P3());
  for ( size_t i = 1; i != objects.size(); i++ ){
    t = objects[i]->GetTri();
    minPt = MinCoords(minPt, MinCoords(t.P0(), t.P1(), t.P2(), t.P3()));
    maxPt = MaxCoords(maxPt, MaxCoords(t.P0(), t.P1(), t.P2(), t.P3()));
  }
  return Rect(minPt, maxPt);
}

FaintSettings DebugRectSettings(){
  return GetRectangleSettings();
}

void AttachPointsFromTri( const Tri& tri, std::vector<Point>& v ){
  v.push_back( tri.P0() );
  v.push_back( tri.P1() );
  v.push_back( tri.P2() );
  v.push_back( tri.P3() );
  v.push_back( MidP0_P1( tri ) );
  v.push_back( MidP0_P2( tri ) );
  v.push_back( MidP1_P3( tri ) );
  v.push_back( MidP2_P3( tri ) );
  v.push_back( CenterPoint( tri ) );
}

std::vector<Point> AttachPointsFromTri( const Tri& tri ){
  std::vector<Point> v;
  AttachPointsFromTri( tri, v );
  return v;
}

std::vector<Object*> Clone(const std::vector<Object*>& old ){
  std::vector<Object*> objects;
  for ( size_t i = 0; i != old.size(); i++ ){
    objects.push_back(old[i]->Clone());
  }
  return objects;
}

void Offset( const std::vector<Object*>& objects, const Point& p ){
  for ( size_t i = 0; i != objects.size(); i++ ){
    OffsetBy( objects[i], p );
  }
}
